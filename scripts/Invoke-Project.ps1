#requires -Version 7.0
[CmdletBinding(SupportsShouldProcess)]
param(
    [Parameter(Position = 0)]
    [ValidateSet('bootstrap', 'configure', 'build', 'test', 'format', 'format:check', 'lint',
        'typecheck', 'deadcode', 'security:audit', 'test:red', 'quality', 'package', 'probe',
        'lint:cpp', 'lint:powershell', 'lint:workflow', 'deadcode:cpp', 'deadcode:copies',
        'security:secrets', 'security:sast', 'security:npm', 'security:osv', 'test:asan', 'test:runner',
        'test:probe', 'test:installer', 'test:address', 'test:live', 'discover', 'discover:classic', 'connect:classic',
        'discover:classic:live', 'connect:classic:live', 'pair:classic')]
    [string]$Task = 'quality',
    [ValidateSet('all', 'changed', 'parser-length', 'module-metadata', 'format', 'owned-header',
        'compiler-types', 'unused-code', 'script-analysis', 'workflow-security', 'secret-scanner',
        'unsafe-copy', 'duplicated-code', 'address-sanitizer', 'dependency-npm', 'dependency-osv',
        'whatif-guard', 'native-exit-propagation', 'probe-arguments', 'classic-connection-claim',
        'private-address-validation', 'uncached-query-loop', 'installer-drive-letter', 'drill-reuse')]
    [string]$Drill = 'all'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$PSNativeCommandUseErrorActionPreference = $false
$projectRoot = Split-Path -Parent $PSScriptRoot
$dependencyRoot = Join-Path $projectRoot '.deps'
$toolRoot = Join-Path $projectRoot '.tools'
$buildRoot = Join-Path $projectRoot 'out/windows'
$dependencyLock = Get-Content -LiteralPath (Join-Path $projectRoot 'dependencies.lock.json') -Raw |
    ConvertFrom-Json
$drillEvidencePath = Join-Path $projectRoot 'reports/red-drills.json'
# The drills test:red runs: 'all', 'changed' (only drills whose inputs changed since their recorded
# pass) or one drill id. quality narrows 'all' to 'changed'; package keeps 'all'.
$script:DrillSelection = $Drill
$script:ReleaseGate = $false
# Besides the file a drill mutates and this runner, the files whose content decides that drill's
# gate result. A folder stands for every file in it. A drill whose gate is not listed cannot run.
$cmakeInputs = @('CMakeLists.txt', 'CMakePresets.json', 'dependencies.lock.json')
$nodeInputs = @('package.json', 'package-lock.json')
$drillGateInputs = @{
    'build' = $cmakeInputs
    'test' = $cmakeInputs + 'tests'
    'test:address' = $cmakeInputs + 'tests'
    'test:asan' = $cmakeInputs + 'tests'
    'test:live' = $cmakeInputs + 'tests/live.Tests.ps1'
    'test:runner' = @('tests/runner.Tests.ps1')
    'test:installer' = @('tests/installer.Tests.ps1')
    'format:check' = @('.clang-format')
    'lint:cpp' = $cmakeInputs + '.clang-tidy'
    'lint:powershell' = @('PSScriptAnalyzerSettings.psd1')
    'lint:workflow' = @('.github/workflows')
    'deadcode:cpp' = $cmakeInputs + '.cppcheck-obs.cfg'
    'deadcode:copies' = $nodeInputs + '.jscpd.json'
    'security:secrets' = @('.gitleaks.toml')
    'security:sast' = @('.semgrep.yml')
    'security:npm' = $nodeInputs
    'security:osv' = $nodeInputs
}

function Invoke-Native {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$Command, [string[]]$ArgumentList = @())
    & $Command @ArgumentList
    if ($LASTEXITCODE -ne 0) {
        throw "Native command failed ($LASTEXITCODE): $Command"
    }
}

function Initialize-BuildEnvironment {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere)) {
        throw 'Visual Studio 2022 C++ build tools are required'
    }
    $installation = & $vswhere -latest -products '*' `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($installation)) {
        throw 'No usable Visual Studio C++ installation found'
    }
    if ($env:VSCMD_ARG_TGT_ARCH -ne 'x64' -or $env:VCToolsVersion -ne '14.44.35207') {
        $env:PATH = (Split-Path -Parent $vswhere) + [IO.Path]::PathSeparator + $env:PATH
        Import-Module (Join-Path $installation 'Common7/Tools/Microsoft.VisualStudio.DevShell.dll')
        Enter-VsDevShell -VsInstallPath $installation -SkipAutomaticLocation `
            -DevCmdArguments '-arch=x64 -host_arch=x64' | Out-Null
    }
    $ninjaDirectory = Join-Path $installation 'Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja'
    if ($ninjaDirectory -notin ($env:PATH -split [IO.Path]::PathSeparator)) {
        $env:PATH = $ninjaDirectory + [IO.Path]::PathSeparator + $env:PATH
    }
    if ([string]::IsNullOrWhiteSpace($env:MGL_OBS_RUNTIME)) {
        $env:MGL_OBS_RUNTIME = Join-Path $env:LOCALAPPDATA 'Programs/obs-studio/bin/64bit'
    }
    if (-not (Test-Path -LiteralPath (Join-Path $env:MGL_OBS_RUNTIME 'obs.dll'))) {
        throw 'Set MGL_OBS_RUNTIME to the installed OBS 32.2.2 bin/64bit directory'
    }
}

function Get-VerifiedArchive {
    [CmdletBinding()]
    param([Parameter(Mandatory)]$Specification, [Parameter(Mandatory)][string]$Destination)
    if (-not (Test-Path -LiteralPath $Destination)) {
        Invoke-WebRequest -Uri $Specification.url -OutFile $Destination
    }
    $actual = (Get-FileHash -LiteralPath $Destination -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $Specification.sha256) {
        throw "Integrity check failed for $Destination; expected pinned SHA-256"
    }
}

function Initialize-Dependency {
    New-Item -ItemType Directory -Path $dependencyRoot, $toolRoot -Force | Out-Null
    $archive = Join-Path $dependencyRoot 'obs-source.tar.gz'
    Get-VerifiedArchive -Specification $dependencyLock.obs -Destination $archive
    $obsSource = Join-Path $dependencyRoot $dependencyLock.obs.directory
    $extractedMarker = Join-Path $obsSource '.mgl-extracted-sha256'
    if (-not (Test-Path -LiteralPath $extractedMarker)) {
        $entries = & tar -tf $archive
        if ($LASTEXITCODE -ne 0 -or @($entries).Count -eq 0) {
            throw 'OBS source archive cannot be listed'
        }
        foreach ($entry in $entries) {
            $resolved = [IO.Path]::GetFullPath((Join-Path $dependencyRoot $entry))
            if (-not $resolved.StartsWith($obsSource + [IO.Path]::DirectorySeparatorChar,
                    [StringComparison]::OrdinalIgnoreCase) -and $resolved -ne $obsSource) {
                throw 'OBS archive contains a path outside its pinned source directory'
            }
        }
        # Python's data filter validates link targets and copies link contents on Windows
        # when symlink creation is unavailable; Windows tar cannot extract these helpers.
        $extractCode = 'import sys,tarfile; a=tarfile.open(sys.argv[1]); a.extractall(sys.argv[2],filter="data"); a.close()'
        Invoke-Native -Command python -ArgumentList @('-c', $extractCode, $archive, $dependencyRoot)
        Set-Content -LiteralPath $extractedMarker -Value $dependencyLock.obs.sha256 -Encoding utf8
    }
    $sdkRoot = Join-Path $dependencyRoot 'obs-sdk'
    $sdkBuild = Join-Path $dependencyRoot 'obs-sdk-build'
    Invoke-Native -Command cmake -ArgumentList @('-S', $obsSource, '-B', $sdkBuild,
        '-G', 'Visual Studio 17 2022', '-A', 'x64,version=10.0.26100.0',
        '-DENABLE_BROWSER=OFF', '-DENABLE_FRONTEND=OFF', '-DENABLE_PLUGINS=OFF',
        '-DENABLE_SCRIPTING=OFF', '-DOBS_VERSION_OVERRIDE=32.2.2', "-DCMAKE_INSTALL_PREFIX=$sdkRoot")
    Invoke-Native -Command cmake -ArgumentList @('--build', $sdkBuild, '--target',
        'obs-frontend-api', '--config', 'RelWithDebInfo', '--parallel', '2')
    Invoke-Native -Command cmake -ArgumentList @('--install', $sdkBuild, '--component',
        'Development', '--config', 'RelWithDebInfo')

    $cppcheckRoot = Join-Path $toolRoot 'cppcheck'
    if (-not (Test-Path -LiteralPath $cppcheckRoot)) {
        $installer = Join-Path $toolRoot 'cppcheck.msi'
        Get-VerifiedArchive -Specification $dependencyLock.cppcheck -Destination $installer
        # Administrative extraction copies a pinned tool into this project; it does not install it.
        $process = Start-Process -FilePath msiexec.exe -ArgumentList @('/a', "`"$installer`"",
            '/qn', "TARGETDIR=`"$cppcheckRoot`"") -WindowStyle Hidden -Wait -PassThru
        if ($process.ExitCode -ne 0) {
            throw "Cppcheck extraction failed with exit $($process.ExitCode)"
        }
    }
    # The WebView2 SDK (headers and the static loader) for the setup window. The NuGet package is
    # a zip; PowerShell 7's Expand-Archive refuses entries that would land outside the destination.
    $webview = Join-Path $dependencyRoot $dependencyLock.webview2.directory
    if (-not (Test-Path -LiteralPath (Join-Path $webview 'build/native/include/WebView2.h'))) {
        $package = Join-Path $dependencyRoot "$($dependencyLock.webview2.directory).nupkg"
        Get-VerifiedArchive -Specification $dependencyLock.webview2 -Destination $package
        Expand-Archive -LiteralPath $package -DestinationPath $webview -Force
    }
    Invoke-Native -Command npm -ArgumentList @('ci', '--ignore-scripts', '--no-fund', '--no-audit')
}

function Get-OwnedCppFile {
    [OutputType([System.IO.FileInfo])]
    param()
    Get-ChildItem -LiteralPath (Join-Path $projectRoot 'src'), (Join-Path $projectRoot 'include'),
    (Join-Path $projectRoot 'tests') -Recurse -File |
        Where-Object { $_.Extension -in '.c', '.cpp', '.hpp' }
}

function Invoke-StaticAnalysis {
    $sources = Get-OwnedCppFile | Where-Object Extension -EQ '.cpp'
    $failedSources = @()
    foreach ($source in $sources) {
        try {
            Invoke-Native -Command clang-tidy -ArgumentList @($source.FullName, '-p', $buildRoot)
        } catch {
            $failedSources += $source.Name
        }
    }
    if ($failedSources.Count -gt 0) {
        throw "clang-tidy rejected: $($failedSources -join ', ')"
    }
}

function Invoke-DeadCodeAnalysis {
    $tools = @(Get-ChildItem -LiteralPath (Join-Path $toolRoot 'cppcheck') -Recurse -File `
            -Filter cppcheck.exe)
    if ($tools.Count -ne 1) {
        throw 'Pinned local Cppcheck is unavailable; run bootstrap'
    }
    $database = Get-Content -LiteralPath (Join-Path $buildRoot 'compile_commands.json') -Raw |
        ConvertFrom-Json
    foreach ($unit in $database) {
        # Cppcheck does not consume MSVC /external:I or compiler builtin macros.
        # Keep the actual per-file standard/defines/paths and normalize only that spelling.
        $unit.command = $unit.command.Replace('/external:I', '-I').Replace('-external:I', '-I') +
        ' -D_WIN32=1 -D_WIN64=1 -D_MSC_VER=1944 -D_M_X64=100'
    }
    $analysisDatabase = Join-Path $buildRoot 'cppcheck_compile_commands.json'
    $database | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $analysisDatabase -Encoding utf8
    $externalHeaders = $dependencyRoot.Replace('\', '/') + '/*'
    # Cppcheck uses its std/windows models. Compiler + clang-tidy validate real system
    # headers; missing quoted owned headers remain errors. Third-party headers are not owned code.
    Invoke-Native -Command $tools[0].FullName -ArgumentList @('--enable=all',
        '--error-exitcode=1', '--platform=win64', '--inline-suppr',
        '--library=windows,.cppcheck-obs.cfg', '--check-level=exhaustive',
        '--suppress=missingIncludeSystem', "--suppress=*:$externalHeaders",
        "--project=$analysisDatabase")
}

function Invoke-SecurityAnalysis {
    foreach ($gate in @('security:secrets', 'security:sast', 'security:npm', 'security:osv')) {
        Invoke-Task -Name $gate
    }
    Invoke-Native -Command gitleaks -ArgumentList @('git', '--redact', '--no-banner', '.')
}

# Stages the end-user bundle that scripts/Install-MetaGLinkDirect.ps1 installs from, then zips it
# with a SHA-256 file: dist/meta-g-link-direct-<version>-windows-x64.zip(.sha256).
function Build-ReleaseBundle {
    $cmake = Get-Content -LiteralPath (Join-Path $projectRoot 'CMakeLists.txt') -Raw
    $match = [regex]::Match($cmake, 'project\(meta_g_link_direct VERSION (\d+\.\d+\.\d+)')
    if (-not $match.Success) {
        throw 'Project version not found in CMakeLists.txt'
    }
    $name = "meta-g-link-direct-$($match.Groups[1].Value)-windows-x64"
    $distRoot = Join-Path $projectRoot 'dist'
    $stage = Join-Path $distRoot $name
    if (Test-Path -LiteralPath $stage) {
        Remove-Item -LiteralPath $stage -Recurse -Force
    }
    $layout = [ordered]@{
        'plugin/meta-g-link-direct.dll' = Join-Path $buildRoot 'meta-g-link-direct.dll'
        'plugin/locale/en-US.ini' = Join-Path $projectRoot 'data/locale/en-US.ini'
        'tools/meta-g-link-probe.exe' = Join-Path $buildRoot 'meta-g-link-probe.exe'
        'tools/meta-g-link-setup.exe' = Join-Path $buildRoot 'meta-g-link-setup.exe'
        'Install-MetaGLinkDirect.ps1' = Join-Path $PSScriptRoot 'Install-MetaGLinkDirect.ps1'
        'README.txt' = Join-Path $projectRoot 'packaging/README.txt'
        'LICENSE' = Join-Path $projectRoot 'LICENSE'
        'THIRD_PARTY_NOTICES.md' = Join-Path $projectRoot 'THIRD_PARTY_NOTICES.md'
    }
    foreach ($entry in $layout.GetEnumerator()) {
        if (-not (Test-Path -LiteralPath $entry.Value)) {
            throw "Release input missing: $($entry.Value)"
        }
        $target = Join-Path $stage $entry.Key
        New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
        Copy-Item -LiteralPath $entry.Value -Destination $target
    }
    # Batch files must reach users with CRLF line endings whatever the checkout settings are.
    foreach ($batch in @('Install.cmd', 'Uninstall.cmd')) {
        $lines = Get-Content -LiteralPath (Join-Path $projectRoot "packaging/$batch")
        [IO.File]::WriteAllText((Join-Path $stage $batch), (($lines -join "`r`n") + "`r`n"),
            [Text.UTF8Encoding]::new($false))
    }
    $zip = Join-Path $distRoot "$name.zip"
    Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip -Force
    $hash = (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash.ToLowerInvariant()
    [IO.File]::WriteAllText("$zip.sha256", "$hash  $name.zip`n", [Text.UTF8Encoding]::new($false))
    Write-Output "Release bundle: $zip"
    Write-Output "SHA-256: $hash"
}

function Invoke-DrillCheck {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$Name)
    $output = & pwsh -NoProfile -File $PSCommandPath $Name 2>&1 | Out-String
    [PSCustomObject]@{ Code = $LASTEXITCODE; Output = $output }
}

# SHA-256 over the relative paths and contents of a drill's inputs. A folder stands for every file in
# it, and a missing input still counts, so creating or deleting an input changes the result.
function Get-InputFingerprint {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$Root, [Parameter(Mandatory)][string[]]$Path)
    $entries = [Collections.Generic.List[string]]::new()
    foreach ($relative in @($Path | Select-Object -Unique)) {
        $full = Join-Path $Root $relative
        if (Test-Path -LiteralPath $full -PathType Container) {
            $files = @(Get-ChildItem -LiteralPath $full -Recurse -File)
        } elseif (Test-Path -LiteralPath $full -PathType Leaf) {
            $files = @(Get-Item -LiteralPath $full)
        } else {
            $entries.Add("$relative`tmissing")
            continue
        }
        foreach ($file in $files) {
            $name = [IO.Path]::GetRelativePath($Root, $file.FullName).Replace('\', '/')
            $entries.Add("$name`t" + (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant())
        }
    }
    $sorted = $entries.ToArray()
    [Array]::Sort($sorted, [StringComparer]::Ordinal)
    $bytes = [Text.Encoding]::UTF8.GetBytes(($sorted -join "`n"))
    return [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($bytes)).ToLowerInvariant()
}

# The recorded result of a drill that passed with exactly these inputs, or $null when it must run.
function Get-ReusableDrillResult {
    [CmdletBinding()]
    param([AllowEmptyCollection()][object[]]$Recorded = @(), [Parameter(Mandatory)][string]$Id,
        [Parameter(Mandatory)][string]$Fingerprint)
    $previous = @($Recorded | Where-Object { $_.id -eq $Id }) | Select-Object -First 1
    if ($null -eq $previous -or -not $previous.PSObject.Properties['inputs_sha256']) {
        return $null
    }
    $passed = $previous.baseline_exit -eq 0 -and $previous.mutated_exit -ne 0 -and $previous.restored_exit -eq 0
    if ($passed -and $previous.inputs_sha256 -eq $Fingerprint) {
        return $previous
    }
    return $null
}

# Writes this run's drill results. A single-drill run keeps the other drills' recorded results.
function Save-DrillEvidence {
    $handled = @($script:DrillEvidence | ForEach-Object { $_.id })
    $kept = @(if ($script:DrillSelection -notin @('all', 'changed')) {
            $script:RecordedDrills | Where-Object { $_.id -notin $handled }
        })
    ConvertTo-Json -InputObject @($kept + $script:DrillEvidence) -Depth 5 |
        Set-Content -LiteralPath $drillEvidencePath -Encoding utf8
}

function Invoke-RestoredDrill {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$Id, [Parameter(Mandatory)][string]$Path,
        [Parameter(Mandatory)][scriptblock]$Mutate, [Parameter(Mandatory)][string]$Gate,
        [Parameter(Mandatory)][string]$ExpectedDiagnostic)
    if ($script:DrillSelection -notin @('all', 'changed') -and $script:DrillSelection -ne $Id) {
        return
    }
    if (-not $drillGateInputs.ContainsKey($Gate)) {
        throw "Red drill $Id uses gate $Gate, which declares no drill inputs"
    }
    $relative = [IO.Path]::GetRelativePath($projectRoot, $Path).Replace('\', '/')
    $runner = [IO.Path]::GetRelativePath($projectRoot, $PSCommandPath).Replace('\', '/')
    $inputs = @($relative, $runner) + $drillGateInputs[$Gate]
    $fingerprint = Get-InputFingerprint -Root $projectRoot -Path $inputs
    if ($script:DrillSelection -eq 'changed') {
        $previous = Get-ReusableDrillResult -Recorded $script:RecordedDrills -Id $Id -Fingerprint $fingerprint
        if ($previous) {
            $script:DrillEvidence.Add($previous)
            $script:DrillsReused++
            Write-Output "UNCHANGED drill $Id (inputs match its pass recorded $($previous.recorded_at))"
            return
        }
    }
    $before = [IO.File]::ReadAllBytes($Path)
    $beforeHash = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
    $baseline = Invoke-DrillCheck -Name $Gate
    if ($baseline.Code -ne 0) {
        Write-Output $baseline.Output
        throw "Red drill baseline failed: $Id"
    }
    $mutated = $null
    $mutatedHash = $null
    $restored = $null
    try {
        $original = [Text.Encoding]::UTF8.GetString($before)
        $changed = & $Mutate $original
        if ($changed -eq $original) {
            throw 'Red-drill mutation did not change the intended input'
        }
        [IO.File]::WriteAllText($Path, $changed, [Text.UTF8Encoding]::new($false))
        $mutatedHash = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
        $mutated = Invoke-DrillCheck -Name $Gate
    } finally {
        [IO.File]::WriteAllBytes($Path, $before)
        $afterHash = (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
        if ($afterHash -ne $beforeHash) {
            throw 'Red-drill restoration hash differs'
        }
        $restored = Invoke-DrillCheck -Name $Gate
    }
    $logPath = Join-Path $projectRoot "reports/red/$Id.txt"
    @("BASELINE EXIT $($baseline.Code)", $baseline.Output, "MUTATED EXIT $($mutated.Code)",
        $mutated.Output, "RESTORED EXIT $($restored.Code)", $restored.Output,
        "RESTORATION SHA256 $afterHash") | Set-Content -LiteralPath $logPath -Encoding utf8
    if ($mutated.Code -eq 0 -or $mutated.Output -notmatch $ExpectedDiagnostic -or $restored.Code -ne 0) {
        throw "Red drill failed or survived: $Id. Inspect $logPath"
    }
    $observed = ($mutated.Output -split "`n" | Where-Object { $_ -match $ExpectedDiagnostic } |
            Select-Object -First 1).Trim()
    $script:DrillEvidence.Add([PSCustomObject]@{
            id = $Id; gate = $Gate; baseline_exit = $baseline.Code; mutated_exit = $mutated.Code
            restored_exit = $restored.Code; expected_diagnostic = $ExpectedDiagnostic
            observed_diagnostic = $observed; path = $relative; before = $beforeHash
            mutated = $mutatedHash; after = $afterHash; log = "reports/red/$Id.txt"
            inputs = @($inputs | Select-Object -Unique); inputs_sha256 = $fingerprint
            recorded_at = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
        })
    Save-DrillEvidence
    Write-Output "PASS drill $Id (green 0, intended red $($mutated.Code), restored 0)"
}

function Invoke-RedDrillSet {
    $script:DrillEvidence = [Collections.Generic.List[object]]::new()
    $script:DrillsReused = 0
    $script:RecordedDrills = @(if (Test-Path -LiteralPath $drillEvidencePath) {
            Get-Content -LiteralPath $drillEvidencePath -Raw | ConvertFrom-Json
        })
    New-Item -ItemType Directory -Path (Join-Path $projectRoot 'reports/red') -Force | Out-Null
    Invoke-RestoredDrill -Id parser-length -Path (Join-Path $projectRoot 'src/protocol/sdu.cpp') -Mutate {
        param($text)
        $text.Replace('DataxHeader = 4;', 'DataxHeader = 5;')
    } -Gate test -ExpectedDiagnostic 'DataX empty has 4 bytes'

    Invoke-RestoredDrill -Id module-metadata -Path (Join-Path $projectRoot 'src/plugin/module.c') -Mutate {
        param($text)
        $text.Replace('return "Meta G Link Direct";', 'return "Broken module name";')
    } -Gate test -ExpectedDiagnostic 'OBS module metadata does not match'

    Invoke-RestoredDrill -Id probe-arguments -Path (Join-Path $projectRoot 'src/probe/main.cpp') -Mutate {
        param($text)
        $text.Replace('return InvalidArguments;', 'return InvalidArguments * EXIT_SUCCESS;')
    } -Gate test -ExpectedDiagnostic 'invalid probe arguments must fail'

    Invoke-RestoredDrill -Id classic-connection-claim -Path (Join-Path $projectRoot 'src/windows/classic.cpp') -Mutate {
        param($text)
        $text.Replace('ClassicDiscovery result{};', 'ClassicDiscovery result{}; result.rfcomm_connected = true;')
    } -Gate test -ExpectedDiagnostic 'Classic discovery must not claim an RFCOMM connection'

    Invoke-RestoredDrill -Id private-address-validation -Path (Join-Path $projectRoot 'src/protocol/address.cpp') -Mutate {
        param($text)
        $text.Replace('address == 0 || address == BroadcastAddress', 'address == BroadcastAddress')
    } -Gate 'test:address' -ExpectedDiagnostic 'Malformed/reserved Bluetooth address accepted'

    Invoke-RestoredDrill -Id uncached-query-loop -Path (Join-Path $projectRoot 'src/windows/classic.cpp') -Mutate {
        param($text)
        $text.Replace('for (const auto address : addresses)', 'for (const auto address : std::vector<std::uint64_t>{})')
    } -Gate 'test:live' -ExpectedDiagnostic 'Live discovery must query available Classic peers'

    Invoke-RestoredDrill -Id format -Path (Join-Path $projectRoot 'src/protocol/sdu.cpp') -Mutate {
        param($text)
        $text + "`nint red_drill_format( ){return 1;}`n"
    } -Gate 'format:check' -ExpectedDiagnostic 'clang-format-violations'

    Invoke-RestoredDrill -Id owned-header -Path (Join-Path $projectRoot 'include/mgl/sdu.hpp') -Mutate {
        param($text)
        $text + "`ninline int red_drill_global = 7;`n"
    } -Gate 'lint:cpp' -ExpectedDiagnostic 'cppcoreguidelines-avoid-non-const-global-variables'

    Invoke-RestoredDrill -Id compiler-types -Path (Join-Path $projectRoot 'src/protocol/sdu.cpp') -Mutate {
        param($text)
        $text + "`nconstexpr int RedDrillType = `"invalid integer`";`n"
    } -Gate build -ExpectedDiagnostic 'cannot initialize|cannot convert|incompatible'

    Invoke-RestoredDrill -Id unused-code -Path (Join-Path $projectRoot 'src/protocol/sdu.cpp') -Mutate {
        param($text)
        $text + "`nint red_drill_unused_function() { return 7; }`n"
    } -Gate 'deadcode:cpp' -ExpectedDiagnostic 'unusedFunction'

    Invoke-RestoredDrill -Id script-analysis -Path $PSCommandPath -Mutate {
        param($text)
        $text + "`nfunction Badverb-RedDrill { }`n"
    } -Gate 'lint:powershell' -ExpectedDiagnostic 'PSUseApprovedVerbs'

    Invoke-RestoredDrill -Id workflow-security -Path (Join-Path $projectRoot '.github/workflows/quality.yml') -Mutate {
        param($text)
        $text.Replace('actions/checkout@d23441a48e516b6c34aea4fa41551a30e30af803', 'actions/checkout@v6')
    } -Gate 'lint:workflow' -ExpectedDiagnostic 'unpinned-uses'

    Invoke-RestoredDrill -Id secret-scanner -Path (Join-Path $projectRoot 'src/protocol/sdu.cpp') -Mutate {
        param($text)
        # Gitleaks's AWS pattern requires base32 A-Z2-7, not hexadecimal 0-9A-F.
        $alphabet = [Collections.Generic.List[char]]::new('ABCDEFGHIJKLMNOPQRSTUVWXYZ234567'.ToCharArray())
        $suffix = for ($index = 0; $index -lt 16; $index++) {
            $selected = [Security.Cryptography.RandomNumberGenerator]::GetInt32($alphabet.Count)
            $alphabet[$selected]
            $alphabet.RemoveAt($selected)
        }
        $canary = 'AKIA' + (-join $suffix)
        $text + "`n// aws_access_key_id = $canary`n"
    } -Gate 'security:secrets' -ExpectedDiagnostic 'leaks found'

    Invoke-RestoredDrill -Id unsafe-copy -Path (Join-Path $projectRoot 'src/protocol/sdu.cpp') -Mutate {
        param($text)
        $text + "`nvoid red_drill_copy(char* d, const char* s) { strcpy(d, s); }`n"
    } -Gate 'security:sast' -ExpectedDiagnostic 'mgl-unsafe-string-copy'

    Invoke-RestoredDrill -Id duplicated-code -Path (Join-Path $projectRoot 'src/protocol/sdu.cpp') -Mutate {
        param($text)
        $start = $text.IndexOf('FramePrefix inspectFramePrefix')
        $end = $text.LastIndexOf('} // namespace mgl')
        $duplicate = $text.Substring($start, $end - $start).Replace('inspectFramePrefix', 'inspectDuplicateFramePrefix')
        $text.Insert($end, $duplicate)
    } -Gate 'deadcode:copies' -ExpectedDiagnostic 'Found [1-9][0-9]* clones|threshold'

    Invoke-RestoredDrill -Id address-sanitizer -Path (Join-Path $projectRoot 'tests/sdu_tests.cpp') -Mutate {
        param($text)
        $text = $text.Replace('#include <array>', "#include <array>`n#include <memory>")
        $text.Replace('        testKnownVectors();', @'
        const auto storage = std::make_unique<int[]>(1);
        volatile std::size_t invalid_index = 1;
        std::cout << storage[invalid_index] << '\n';
        testKnownVectors();
'@)
    } -Gate 'test:asan' -ExpectedDiagnostic 'AddressSanitizer: heap-buffer-overflow'

    $vulnerableLockMutation = {
        param($text)
        $lock = $text | ConvertFrom-Json -AsHashtable
        $lock.packages[''].devDependencies['lodash'] = '4.17.20'
        $lock.packages['node_modules/lodash'] = @{
            version = '4.17.20'; resolved = 'https://registry.npmjs.org/lodash/-/lodash-4.17.20.tgz'
            dev = $true; license = 'MIT'
        }
        $lock | ConvertTo-Json -Depth 25
    }
    foreach ($scanner in @('npm', 'osv')) {
        Invoke-RestoredDrill -Id "dependency-$scanner" -Path (Join-Path $projectRoot 'package-lock.json') `
            -Mutate $vulnerableLockMutation -Gate "security:$scanner" `
            -ExpectedDiagnostic 'GHSA-35jh-r3h4-6jhm'
    }

    Invoke-RestoredDrill -Id whatif-guard -Path $PSCommandPath -Mutate {
        param($text)
        $text.Replace('if (-not $PSCmdlet.ShouldProcess($projectRoot, "Run project task $Task"))',
            'if ($false)')
    } -Gate 'test:runner' -ExpectedDiagnostic 'Expected strings to be the same'

    Invoke-RestoredDrill -Id native-exit-propagation -Path $PSCommandPath -Mutate {
        param($text)
        $text.Replace('if ($LASTEXITCODE -ne 0) {', 'if ($false) {')
    } -Gate 'test:runner' -ExpectedDiagnostic 'Expected 0 to be different from the actual value'

    Invoke-RestoredDrill -Id installer-drive-letter -Path (Join-Path $projectRoot 'scripts/Install-MetaGLinkDirect.ps1') -Mutate {
        param($text)
        $text.Replace('(Get-KnownFolder -Folder CommonApplicationData)', "'C:\ProgramData'")
    } -Gate 'test:installer' -ExpectedDiagnostic 'because the installer must not assume a drive letter'

    Invoke-RestoredDrill -Id drill-reuse -Path $PSCommandPath -Mutate {
        param($text)
        $text.Replace('$passed -and $previous.inputs_sha256 -eq $Fingerprint', '$passed')
    } -Gate 'test:runner' -ExpectedDiagnostic 'because a drill whose inputs changed must run again'
    $expectedDrillCount = if ($script:DrillSelection -in @('all', 'changed')) { 22 } else { 1 }
    if ($script:DrillEvidence.Count -ne $expectedDrillCount) {
        throw 'Red-drill discovery did not match the requested scope'
    }
    Save-DrillEvidence
    $ran = $script:DrillEvidence.Count - $script:DrillsReused
    Write-Output "Red drills: $ran run, $($script:DrillsReused) unchanged since their recorded pass"
}

function Invoke-Task {
    [CmdletBinding()]
    param([Parameter(Mandatory)][string]$Name)
    switch ($Name) {
        bootstrap { Initialize-Dependency }
        configure { Invoke-Native -Command cmake -ArgumentList @('--preset', 'windows', '--fresh') }
        build { Invoke-Native -Command cmake -ArgumentList @('--build', '--preset', 'windows') }
        typecheck { Invoke-Task -Name build }
        test {
            Invoke-Task -Name build
            Invoke-Native -Command ctest -ArgumentList @('--preset', 'windows')
            Invoke-Task -Name 'test:runner'
            Invoke-Task -Name 'test:probe'
            Invoke-Task -Name 'test:installer'
        }
        'test:runner' {
            $result = Invoke-Pester -Path (Join-Path $projectRoot 'tests/runner.Tests.ps1') `
                -PassThru -Output Detailed
            if ($result.TotalCount -eq 0 -or $result.FailedCount -ne 0 -or $result.PassedCount -ne 5) {
                throw 'Pester runner tests failed or discovery count changed'
            }
        }
        'test:probe' {
            $result = Invoke-Pester -Path (Join-Path $projectRoot 'tests/probe.Tests.ps1') `
                -PassThru -Output Detailed
            if ($result.TotalCount -ne 21 -or $result.FailedCount -ne 0 -or $result.PassedCount -ne 21) {
                throw 'Pester probe tests failed or discovery count changed'
            }
        }
        'test:installer' {
            $result = Invoke-Pester -Path (Join-Path $projectRoot 'tests/installer.Tests.ps1') `
                -PassThru -Output Detailed
            if ($result.TotalCount -ne 4 -or $result.FailedCount -ne 0 -or $result.PassedCount -ne 4) {
                throw 'Pester installer tests failed or discovery count changed'
            }
        }
        'test:address' {
            Invoke-Task -Name build
            Invoke-Native -Command ctest -ArgumentList @('--preset', 'windows', '-R', '^private-address$')
        }
        'test:live' {
            Invoke-Task -Name build
            $result = Invoke-Pester -Path (Join-Path $projectRoot 'tests/live.Tests.ps1') `
                -PassThru -Output Detailed
            if ($result.TotalCount -ne 1 -or $result.FailedCount -ne 0 -or $result.PassedCount -ne 1) {
                throw 'Pester live SDP tests failed or discovery count changed'
            }
        }
        'test:asan' {
            Invoke-Native -Command cmake -ArgumentList @('--preset', 'windows-asan')
            Invoke-Native -Command cmake -ArgumentList @('--build', '--preset', 'windows-asan')
            Invoke-Native -Command ctest -ArgumentList @('--preset', 'windows-asan')
        }
        format {
            Invoke-Native -Command clang-format -ArgumentList (@('-i') + @(Get-OwnedCppFile).FullName)
        }
        'format:check' {
            Invoke-Native -Command clang-format -ArgumentList (@('--dry-run', '--Werror') +
                @(Get-OwnedCppFile).FullName)
        }
        lint {
            foreach ($gate in @('lint:cpp', 'lint:powershell', 'lint:workflow')) { Invoke-Task -Name $gate }
        }
        'lint:cpp' { Invoke-StaticAnalysis }
        'lint:powershell' {
            $findings = @(Get-ChildItem -LiteralPath $PSScriptRoot, (Join-Path $projectRoot 'tests') `
                    -Recurse -File -Filter '*.ps1' | Invoke-ScriptAnalyzer `
                    -Settings (Join-Path $projectRoot 'PSScriptAnalyzerSettings.psd1') -ErrorAction Stop)
            if ($findings.Count -gt 0) {
                $findings | Format-Table -AutoSize | Out-String | Write-Output
                throw 'PSScriptAnalyzer rejected owned scripts'
            }
        }
        'lint:workflow' {
            Invoke-Native -Command actionlint
            Invoke-Native -Command zizmor -ArgumentList @('--no-progress', '--pedantic', '--format', 'plain',
                (Join-Path $projectRoot '.github/workflows'))
        }
        deadcode {
            Invoke-Task -Name 'deadcode:cpp'
            Invoke-Task -Name 'deadcode:copies'
        }
        'deadcode:cpp' { Invoke-DeadCodeAnalysis }
        'deadcode:copies' {
            Invoke-Native -Command npx -ArgumentList @('--no-install', 'jscpd', '--config', '.jscpd.json', '--fail-on-empty')
        }
        'security:secrets' { Invoke-Native -Command gitleaks -ArgumentList @('dir', '--redact', '--no-banner', '.') }
        'security:sast' {
            Invoke-Native -Command semgrep -ArgumentList @('scan', '--error', '--metrics=off',
                '--disable-version-check', '--config', '.semgrep.yml', 'src', 'include')
        }
        'security:npm' { Invoke-Native -Command npm -ArgumentList @('audit', '--audit-level=moderate') }
        'security:osv' { Invoke-Native -Command osv-scanner -ArgumentList @('scan', 'source', '--recursive', '.') }
        'security:audit' { Invoke-SecurityAnalysis }
        'test:red' { Invoke-RedDrillSet }
        quality {
            # Every gate runs on the whole tree. Day to day, only the drills whose inputs changed
            # since their recorded pass run again; a release reruns all of them.
            if (-not $script:ReleaseGate) {
                $script:DrillSelection = 'changed'
            }
            foreach ($gate in @('configure', 'build', 'format:check', 'lint', 'deadcode',
                    'security:audit', 'test', 'test:live', 'test:red')) {
                Write-Output "Gate: $gate"
                Invoke-Task -Name $gate
            }
        }
        package {
            $script:ReleaseGate = $true
            Invoke-Task -Name quality
            Build-ReleaseBundle
        }
        probe { Invoke-Native -Command (Join-Path $buildRoot 'meta-g-link-probe.exe') }
        discover {
            Invoke-Native -Command (Join-Path $buildRoot 'meta-g-link-probe.exe') -ArgumentList '--discover'
        }
        'discover:classic' {
            Invoke-Native -Command (Join-Path $buildRoot 'meta-g-link-probe.exe') -ArgumentList '--discover-classic'
        }
        'connect:classic' {
            Invoke-Native -Command (Join-Path $buildRoot 'meta-g-link-probe.exe') -ArgumentList '--connect-classic'
        }
        'discover:classic:live' {
            Invoke-Native -Command (Join-Path $buildRoot 'meta-g-link-probe.exe') -ArgumentList '--discover-classic-live'
        }
        'connect:classic:live' {
            Invoke-Native -Command (Join-Path $buildRoot 'meta-g-link-probe.exe') -ArgumentList '--connect-classic-live'
        }
        'pair:classic' {
            Invoke-Native -Command (Join-Path $buildRoot 'meta-g-link-probe.exe') -ArgumentList '--pair-classic'
        }
        default { throw "Unknown project task: $Name" }
    }
}

if ($Drill -ne 'all' -and $Task -ne 'test:red') {
    throw 'A drill selection is valid only for test:red; quality reruns changed drills and package all of them'
}
if (-not $PSCmdlet.ShouldProcess($projectRoot, "Run project task $Task")) {
    return
}
Push-Location -LiteralPath $projectRoot
try {
    if ($Task -in @('bootstrap', 'configure', 'build', 'typecheck', 'test', 'test:asan',
            'lint', 'lint:cpp', 'test:address', 'test:live', 'quality', 'package')) {
        Initialize-BuildEnvironment
    }
    Invoke-Task -Name $Task
} catch {
    Write-Error -Message $_.Exception.Message -ErrorAction Continue
    exit 1
} finally {
    Pop-Location
}
