#requires -Version 5.1
<#
.SYNOPSIS
Installs the Meta G Link Direct OBS plugin and its setup tool from an unpacked bundle.

.DESCRIPTION
Run from a bundle folder that contains plugin\meta-g-link-direct.dll,
plugin\locale\en-US.ini, tools\meta-g-link-setup.exe (the setup window) and
tools\meta-g-link-probe.exe (the setup tool's command line). The script:
  1. finds OBS Studio 32 or newer and checks its FFmpeg and the Visual C++ runtime,
  2. waits for OBS to be closed (it never closes OBS itself),
  3. copies the plugin into the folder OBS loads plugins from
     (%ProgramData%\obs-studio\plugins\meta-g-link-direct, or the OBS folder for a
     portable OBS), removes copies an earlier install left in other layouts, and
     elevates only that step when a folder is protected,
  4. copies the setup window and the setup tool to %LOCALAPPDATA%\MetaGLinkDirect\bin,
  5. imports meta-tokens.txt, if present, into the DPAPI-sealed token store; it overrides the
     Meta AI app tokens built into the setup tool, for when Meta rotates them,
  6. optionally opens the setup window for the one-time glasses setup (Meta login + ownership
     change).

The ownership change makes this PC the owner of the glasses; the Meta AI phone
app, or another PC, loses access until the glasses are set up there again.

.PARAMETER ObsPath
OBS Studio installation folder (the one containing bin\64bit\obs64.exe), on any
drive. When omitted, the installer looks for a running OBS, the OBS installer's
registry entry, Program Files, the per-user Programs folder and every Steam
library folder.

.PARAMETER TokenFile
Optional Meta app token file to import over the built-in tokens. Defaults to meta-tokens.txt
next to this script; nothing is imported when it is absent.

.PARAMETER Setup
Run the glasses setup after installing without asking first.

.PARAMETER Uninstall
Remove the plugin and the setup tool. The sealed identity and token store are
kept; delete %LOCALAPPDATA%\MetaGLinkDirect to forget them.

.PARAMETER CopyOnly
Internal: used by the elevated child process to copy only the plugin files.

.PARAMETER RemoveOnly
Internal: used by the elevated child process to remove only the plugin files.
#>
[CmdletBinding(SupportsShouldProcess)]
param(
    [string]$ObsPath,
    [string]$TokenFile,
    [switch]$Setup,
    [switch]$Uninstall,
    [switch]$CopyOnly,
    [switch]$RemoveOnly
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

# Windows, Program Files, ProgramData and user profiles can each be on any drive, so every system
# folder comes from Windows rather than from a drive letter.
function Get-KnownFolder {
    param([Parameter(Mandatory)][Environment+SpecialFolder]$Folder)
    $path = [Environment]::GetFolderPath($Folder)
    if ([string]::IsNullOrWhiteSpace($path)) {
        throw "Windows did not report the $Folder folder."
    }
    return $path
}

$moduleName = 'meta-g-link-direct'
$bundleRoot = $PSScriptRoot
$toolRoot = [IO.Path]::Combine((Get-KnownFolder -Folder LocalApplicationData), 'MetaGLinkDirect', 'bin')
$toolPath = [IO.Path]::Combine($toolRoot, 'meta-g-link-probe.exe')
$setupWindowPath = [IO.Path]::Combine($toolRoot, 'meta-g-link-setup.exe')
$setupTools = @('meta-g-link-setup.exe', 'meta-g-link-probe.exe')
$minimumObsMajor = 32
$testedObsMajors = @(32, 33)
# OBS 33 introduced the flat plugin layout (plugins\<name>\<name>.dll); OBS 32 reads
# plugins\<name>\bin\64bit. OBS 34 stops loading plugins from the Program Files folder. The flat
# layout is what a portable OBS 33 or later reads from its own plugins folder.
$flatLayoutObsMajor = 33
# OBS resolves this folder through the same Windows call (the common application data folder).
$programDataPlugins = [IO.Path]::Combine((Get-KnownFolder -Folder CommonApplicationData), 'obs-studio', 'plugins')
$requiredFfmpeg = @('avcodec-62.dll', 'avutil-60.dll')
$requiredRuntime = @('msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll')
$runtimeUrl = 'https://aka.ms/vs/17/release/vc_redist.x64.exe'

function Write-Step {
    param([Parameter(Mandatory)][string]$Message)
    Write-Information -MessageData $Message -InformationAction Continue
}

function Test-ObsInstallation {
    param([string]$Path)
    if ([string]::IsNullOrWhiteSpace($Path)) {
        return $false
    }
    return Test-Path -LiteralPath ([IO.Path]::Combine($Path, 'bin', '64bit', 'obs64.exe'))
}

# A registry string value, or $null when the key or the value is absent. An empty name reads the
# key's default value.
function Get-RegistryText {
    param([Parameter(Mandatory)][string]$Key, [Parameter(Mandatory)][AllowEmptyString()][string]$Name)
    if (-not (Test-Path -LiteralPath $Key)) {
        return $null
    }
    return (Get-Item -LiteralPath $Key).GetValue($Name)
}

# The OBS folder that holds <obs>\bin\64bit\obs64.exe.
function Get-ObsRootFromExecutable {
    param([Parameter(Mandatory)][string]$Path)
    return Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $Path))
}

# Running OBS processes show where the user's OBS is, wherever it was installed or extracted.
function Get-RunningObsRoot {
    # Loading the CIM module defines aliases, which -WhatIf would otherwise report as actions.
    $processes = & {
        $WhatIfPreference = $false
        @(Get-CimInstance -ClassName Win32_Process -Filter "Name = 'obs64.exe'")
    }
    foreach ($process in $processes) {
        # ExecutablePath is empty for a process this account may not inspect, such as an OBS
        # running as administrator; the other locations are still searched.
        if ($process.ExecutablePath) {
            Get-ObsRootFromExecutable -Path $process.ExecutablePath
        }
    }
}

# Steam's own folder and the library folders, on any drive, that it lists in
# steamapps\libraryfolders.vdf. Paths in that file are quoted with backslashes escaped.
function Get-SteamLibraryFolder {
    param([Parameter(Mandatory)][string]$SteamRoot)
    $folders = [Collections.Generic.List[string]]::new()
    $folders.Add($SteamRoot)
    $manifest = [IO.Path]::Combine($SteamRoot, 'steamapps', 'libraryfolders.vdf')
    if (Test-Path -LiteralPath $manifest) {
        $text = Get-Content -LiteralPath $manifest -Raw
        foreach ($match in [regex]::Matches($text, '"path"\s+"((?:[^"\\]|\\.)*)"')) {
            $folders.Add([regex]::Replace($match.Groups[1].Value, '\\(.)', '$1'))
        }
    }
    return $folders
}

function Get-SteamRoot {
    $roots = @(
        (Get-RegistryText -Key 'HKCU:\Software\Valve\Steam' -Name 'SteamPath'),
        (Get-RegistryText -Key 'HKLM:\SOFTWARE\WOW6432Node\Valve\Steam' -Name 'InstallPath'),
        [IO.Path]::Combine((Get-KnownFolder -Folder ProgramFilesX86), 'Steam'))
    return @($roots | Where-Object { $_ -and (Test-Path -LiteralPath $_ -PathType Container) })
}

function Find-ObsInstallation {
    param([string]$Requested)
    if (-not [string]::IsNullOrWhiteSpace($Requested)) {
        if (-not (Test-ObsInstallation -Path $Requested)) {
            throw "OBS was not found at '$Requested' (expected bin\64bit\obs64.exe)."
        }
        return (Resolve-Path -LiteralPath $Requested).Path
    }
    $candidates = [Collections.Generic.List[string]]::new()
    foreach ($root in @(Get-RunningObsRoot)) {
        $candidates.Add($root)
    }
    foreach ($key in @('HKLM:\SOFTWARE\OBS Studio', 'HKCU:\SOFTWARE\OBS Studio')) {
        $value = Get-RegistryText -Key $key -Name ''
        if ($value) {
            $candidates.Add($value)
        }
    }
    $candidates.Add([IO.Path]::Combine((Get-KnownFolder -Folder ProgramFiles), 'obs-studio'))
    $candidates.Add([IO.Path]::Combine((Get-KnownFolder -Folder LocalApplicationData), 'Programs', 'obs-studio'))
    foreach ($steam in @(Get-SteamRoot)) {
        foreach ($library in @(Get-SteamLibraryFolder -SteamRoot $steam)) {
            $candidates.Add([IO.Path]::Combine($library, 'steamapps', 'common', 'OBS Studio'))
        }
    }
    # The same folder can be reached through several sources, spelled with different case or
    # slashes.
    $seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $found = @(foreach ($candidate in $candidates) {
            if (Test-ObsInstallation -Path $candidate) {
                $full = [IO.Path]::GetFullPath($candidate).TrimEnd([IO.Path]::DirectorySeparatorChar)
                if ($seen.Add($full)) {
                    $full
                }
            }
        })
    if ($found.Count -eq 0) {
        throw 'OBS Studio was not found. Install OBS 32 or newer, or pass -ObsPath "<OBS folder>" (the folder that contains bin\64bit\obs64.exe).'
    }
    if ($found.Count -gt 1) {
        Write-Step "OBS was found in more than one folder ($($found -join '; ')). Using the first; pass -ObsPath to choose another."
    }
    return $found[0]
}

function Get-ObsVersion {
    param([Parameter(Mandatory)][string]$Obs)
    $exe = [IO.Path]::Combine($Obs, 'bin', '64bit', 'obs64.exe')
    $version = "$((Get-Item -LiteralPath $exe).VersionInfo.ProductVersion)"
    $major = 0
    if (-not ($version -match '^(\d+)\.' -and [int]::TryParse($Matches[1], [ref]$major))) {
        throw "Could not read the OBS version from $exe."
    }
    [PSCustomObject]@{ Text = $version; Major = $major }
}

function Test-ObsCompatibility {
    param([Parameter(Mandatory)][string]$Obs)
    $version = Get-ObsVersion -Obs $Obs
    if ($version.Major -lt $minimumObsMajor) {
        throw "Found OBS $($version.Text). This build needs OBS $minimumObsMajor or newer."
    }
    if ($version.Major -notin $testedObsMajors) {
        Write-Step "OBS $($version.Text) is newer than the versions this build was checked with ($($testedObsMajors -join ', ')); continuing."
    }
    $binary = [IO.Path]::Combine($Obs, 'bin', '64bit')
    $missing = @($requiredFfmpeg | Where-Object { -not (Test-Path -LiteralPath ([IO.Path]::Combine($binary, $_))) })
    if ($missing.Count -gt 0) {
        throw "OBS $($version.Text) lacks $($missing -join ', '), which this build decodes video and audio with."
    }
    return $version.Text
}

# A portable OBS keeps its plugins in its own folder and ignores ProgramData.
function Test-ObsPortable {
    param([Parameter(Mandatory)][string]$Obs)
    $markers = @('obs_portable_mode.txt', 'portable_mode.txt')
    return @($markers | Where-Object { Test-Path -LiteralPath ([IO.Path]::Combine($Obs, $_)) }).Count -gt 0
}

function Test-VcRuntime {
    $system = [Environment]::GetFolderPath('System')
    $missing = @($requiredRuntime | Where-Object { -not (Test-Path -LiteralPath ([IO.Path]::Combine($system, $_))) })
    if ($missing.Count -gt 0) {
        throw "The Microsoft Visual C++ x64 runtime is missing ($($missing -join ', ')). Install it from $runtimeUrl and run this again."
    }
}

function Wait-ObsClosed {
    while (Get-Process -Name obs64 -ErrorAction SilentlyContinue) {
        if ($WhatIfPreference) {
            Write-Step 'OBS is running; a real install would wait for it to close.'
            return
        }
        Read-Host -Prompt 'OBS is running. Close OBS, then press Enter' | Out-Null
    }
}

# True when the current user can create files in Path, or in its nearest existing parent when
# Path does not exist yet.
function Test-PathWritable {
    param([Parameter(Mandatory)][string]$Path)
    $directory = $Path
    while ($directory -and -not (Test-Path -LiteralPath $directory -PathType Container)) {
        $directory = Split-Path -Parent $directory
    }
    if (-not $directory) {
        return $false
    }
    # The probe file is removed directly so that -WhatIf cannot leave it behind.
    $probe = [IO.Path]::Combine($directory, '.mgl-write-test')
    try {
        [IO.File]::Create($probe).Dispose()
        [IO.File]::Delete($probe)
        return $true
    } catch [UnauthorizedAccessException] {
        return $false
    }
}

# Where this OBS loads the plugin from, and every other place an earlier install of this plugin
# may have left a copy.
function Get-PluginLayout {
    param([Parameter(Mandatory)][string]$Obs)
    $major = (Get-ObsVersion -Obs $Obs).Major
    $portable = Test-ObsPortable -Obs $Obs
    $legacyDll = [IO.Path]::Combine($Obs, 'obs-plugins', '64bit', "$moduleName.dll")
    $legacyData = [IO.Path]::Combine($Obs, 'data', 'obs-plugins', $moduleName)
    $root = if ($portable) { [IO.Path]::Combine($Obs, 'plugins', $moduleName) }
    else { [IO.Path]::Combine($programDataPlugins, $moduleName) }
    $flatDll = [IO.Path]::Combine($root, "$moduleName.dll")
    $binFolder = [IO.Path]::Combine($root, 'bin')
    $binDll = [IO.Path]::Combine($binFolder, '64bit', "$moduleName.dll")
    $rootData = [IO.Path]::Combine($root, 'data')
    if (-not $portable) {
        # ProgramData is shared by every OBS on the PC, so it gets both layouts: OBS 32 reads
        # bin\64bit and OBS 33 and later prefer the flat copy and skip the other. An OBS upgrade
        # then needs no reinstall, and OBS 32 and 33 can both be installed.
        $layout = @{ Name = 'the ProgramData plugin folder (for OBS 32 and for OBS 33 and later)'
            Dlls = @($flatDll, $binDll); Data = $rootData; Stale = @($legacyDll, $legacyData) }
    } elseif ($major -ge $flatLayoutObsMajor) {
        $layout = @{ Name = "portable OBS's plugin folder"; Dlls = @($flatDll); Data = $rootData
            Stale = @($binFolder, $legacyDll, $legacyData) }
    } else {
        $layout = @{ Name = "portable OBS's legacy plugin folder"; Dlls = @($legacyDll); Data = $legacyData
            Stale = @($flatDll, $binFolder) }
    }
    $layout.Root = $root
    [PSCustomObject]$layout
}

# Every place this plugin may be installed. The ProgramData folder does not depend on OBS, so it
# is removed even when OBS itself is already gone.
function Get-PluginRemovalPath {
    param([string]$Obs)
    $paths = @([IO.Path]::Combine($programDataPlugins, $moduleName))
    if ($Obs) {
        $paths += @([IO.Path]::Combine($Obs, 'plugins', $moduleName),
            [IO.Path]::Combine($Obs, 'obs-plugins', '64bit', "$moduleName.dll"),
            [IO.Path]::Combine($Obs, 'data', 'obs-plugins', $moduleName))
    }
    return $paths
}

function Install-PluginFile {
    [CmdletBinding(SupportsShouldProcess)]
    param([Parameter(Mandatory)][string]$Obs)
    $layout = Get-PluginLayout -Obs $Obs
    $dll = [IO.Path]::Combine($bundleRoot, 'plugin', "$moduleName.dll")
    $locale = [IO.Path]::Combine($bundleRoot, 'plugin', 'locale', 'en-US.ini')
    foreach ($source in @($dll, $locale)) {
        if (-not (Test-Path -LiteralPath $source)) {
            throw "Bundle file missing: $source"
        }
    }
    foreach ($target in $layout.Dlls) {
        $dllFolder = Split-Path -Parent $target
        if ($PSCmdlet.ShouldProcess($dllFolder, "Copy $moduleName.dll")) {
            New-Item -ItemType Directory -Path $dllFolder -Force | Out-Null
            Copy-Item -LiteralPath $dll -Destination $dllFolder -Force
            Unblock-File -LiteralPath $target
        }
    }
    $localeFolder = [IO.Path]::Combine($layout.Data, 'locale')
    if ($PSCmdlet.ShouldProcess($localeFolder, 'Copy en-US.ini')) {
        New-Item -ItemType Directory -Path $localeFolder -Force | Out-Null
        Copy-Item -LiteralPath $locale -Destination $localeFolder -Force
    }
    # A copy in another layout would load instead of, or as well as, this one.
    foreach ($path in $layout.Stale) {
        if ((Test-Path -LiteralPath $path) -and $PSCmdlet.ShouldProcess($path, 'Remove an older copy')) {
            Remove-Item -LiteralPath $path -Recurse -Force
        }
    }
}

function Remove-PluginFile {
    [CmdletBinding(SupportsShouldProcess)]
    param([string]$Obs)
    foreach ($path in (Get-PluginRemovalPath -Obs $Obs)) {
        if ((Test-Path -LiteralPath $path) -and $PSCmdlet.ShouldProcess($path, 'Remove')) {
            Remove-Item -LiteralPath $path -Recurse -Force
        }
    }
}

# Copies or removes the plugin files, elevating only that step when a folder it must change is
# protected, such as an older copy under Program Files.
function Invoke-PluginFileStep {
    [CmdletBinding(SupportsShouldProcess)]
    param(
        [string]$Obs,
        [Parameter(Mandatory)][ValidateSet('Copy', 'Remove')][string]$Mode
    )
    if ($Mode -eq 'Copy') {
        $layout = Get-PluginLayout -Obs $Obs
        Write-Step "Installing into $($layout.Name): $($layout.Root)"
        $stale = @($layout.Stale | Where-Object { Test-Path -LiteralPath $_ })
        $folders = @($layout.Dlls | ForEach-Object { Split-Path -Parent $_ })
        $touched = $folders + @($layout.Data) + $stale
    } else {
        $touched = @(Get-PluginRemovalPath -Obs $Obs | Where-Object { Test-Path -LiteralPath $_ })
    }
    $elevated = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
        [Security.Principal.WindowsBuiltInRole]::Administrator)
    # A dry run writes nothing, not even the permission probe.
    # @() around the whole if keeps an empty result an array in Windows PowerShell 5.1.
    $blocked = @(if (-not ($WhatIfPreference -or $elevated)) {
            $touched | Where-Object { -not (Test-PathWritable -Path $_) }
        })
    if ($WhatIfPreference -or $elevated -or $blocked.Count -eq 0) {
        if ($Mode -eq 'Copy') { Install-PluginFile -Obs $Obs } else { Remove-PluginFile -Obs $Obs }
        return
    }
    $verb = $Mode.ToLowerInvariant()
    if (-not $PSCmdlet.ShouldProcess($Obs, "$Mode the plugin with administrator rights")) {
        return
    }
    # Only the plugin file step runs elevated; the token import and setup stay in this user's
    # context because DPAPI seals the store to the account that runs it.
    Write-Step "A plugin folder is protected; Windows will ask for permission to $verb the plugin."
    $hostPath = (Get-Process -Id $PID).Path
    $childSwitch = if ($Mode -eq 'Copy') { '-CopyOnly' } else { '-RemoveOnly' }
    $arguments = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$PSCommandPath`"", $childSwitch)
    if ($Obs) {
        $arguments += @('-ObsPath', "`"$Obs`"")
    }
    $child = Start-Process -FilePath $hostPath -ArgumentList $arguments -Verb RunAs -Wait -PassThru
    if ($child.ExitCode -ne 0) {
        throw "The elevated plugin $verb step failed (exit $($child.ExitCode))."
    }
}

function Install-SetupTool {
    [CmdletBinding(SupportsShouldProcess)]
    param()
    foreach ($name in $setupTools) {
        $source = [IO.Path]::Combine($bundleRoot, 'tools', $name)
        if (-not (Test-Path -LiteralPath $source)) {
            throw "Bundle file missing: $source"
        }
    }
    foreach ($name in $setupTools) {
        if ($PSCmdlet.ShouldProcess($toolRoot, "Copy $name")) {
            New-Item -ItemType Directory -Path $toolRoot -Force | Out-Null
            Copy-Item -LiteralPath ([IO.Path]::Combine($bundleRoot, 'tools', $name)) -Destination $toolRoot -Force
            Unblock-File -LiteralPath ([IO.Path]::Combine($toolRoot, $name))
        }
    }
}

function Import-TokenFile {
    [CmdletBinding(SupportsShouldProcess)]
    param([Parameter(Mandatory)][string]$Path)
    if (-not (Test-Path -LiteralPath $Path)) {
        Write-Step 'No token file; setup uses the Meta AI app tokens built into the setup tool.'
        return
    }
    if ($PSCmdlet.ShouldProcess('DPAPI token store', "Import tokens from $Path")) {
        # The tool seals the text for this Windows account and never prints it back.
        Get-Content -LiteralPath $Path -Raw | & $toolPath --import-tokens | Out-Null
        if ($LASTEXITCODE -ne 0) {
            throw "Token import failed (exit $LASTEXITCODE)."
        }
        Write-Step 'Meta app tokens imported and sealed for this Windows account.'
    }
}

function Invoke-GlassesSetup {
    [CmdletBinding(SupportsShouldProcess)]
    param()
    if (-not $PSCmdlet.ShouldProcess('the glasses', 'Change the owner to this PC')) {
        return
    }
    # The setup window explains the change of owner and shows the Meta login itself, so nothing is
    # copied; the OBS Set up glasses button opens the same window.
    $setupCancelled = 8
    Write-Step 'The setup window is open; follow it there.'
    $window = Start-Process -FilePath $setupWindowPath -Wait -PassThru
    $exitCode = $window.ExitCode
    if ($exitCode -eq $setupCancelled) {
        Write-Step 'Setup skipped. Run it later from the glasses source properties in OBS (Set up glasses), or with .\Install.cmd -Setup.'
        return
    }
    if ($exitCode -ne 0) {
        throw "The glasses setup did not finish (exit $exitCode). The setup window said which step stopped; run .\Install.cmd -Setup to try again."
    }
    Write-Step 'This PC now owns the glasses.'
}

# The OBS folder, or an empty string when OBS cannot be found; uninstalling still works then.
function Find-ObsInstallationOrNone {
    param([string]$Requested)
    try {
        return Find-ObsInstallation -Requested $Requested
    } catch {
        Write-Step "$($_.Exception.Message) Copies inside an OBS folder cannot be removed."
        return ''
    }
}

# Removes the plugin and the setup tool. The sealed identity is kept.
function Uninstall-Plugin {
    [CmdletBinding(SupportsShouldProcess)]
    param([string]$Requested)
    $obs = Find-ObsInstallationOrNone -Requested $Requested
    if ($obs) {
        Write-Step "OBS: $obs"
        Wait-ObsClosed
    }
    Invoke-PluginFileStep -Obs $obs -Mode Remove
    if ((Test-Path -LiteralPath $toolRoot) -and $PSCmdlet.ShouldProcess($toolRoot, 'Remove')) {
        Remove-Item -LiteralPath $toolRoot -Recurse -Force
    }
    Write-Step 'Removed the plugin and the setup tool. The sealed identity in %LOCALAPPDATA%\MetaGLinkDirect was kept.'
}

try {
    if ($RemoveOnly) {
        Remove-PluginFile -Obs (Find-ObsInstallationOrNone -Requested $ObsPath)
        exit 0
    }
    if ($Uninstall) {
        Uninstall-Plugin -Requested $ObsPath
        exit 0
    }
    $obs = Find-ObsInstallation -Requested $ObsPath
    if ($CopyOnly) {
        Install-PluginFile -Obs $obs
        exit 0
    }
    Write-Step "OBS: $obs"
    Wait-ObsClosed
    $version = Test-ObsCompatibility -Obs $obs
    Write-Step "OBS version $version is compatible."
    Test-VcRuntime
    Invoke-PluginFileStep -Obs $obs -Mode Copy
    Install-SetupTool
    if ([string]::IsNullOrWhiteSpace($TokenFile)) {
        $TokenFile = [IO.Path]::Combine($bundleRoot, 'meta-tokens.txt')
    }
    Import-TokenFile -Path $TokenFile
    Write-Step "Installed. Setup tool: $toolPath"
    $runSetup = $Setup.IsPresent
    if (-not $runSetup -and -not $WhatIfPreference) {
        $answer = Read-Host -Prompt 'Set up the glasses for this PC now? This changes their owner. [y/N]'
        $runSetup = $answer -match '^(y|yes)$'
    }
    if ($runSetup -or $WhatIfPreference) {
        Invoke-GlassesSetup
    }
    Write-Step 'Done. In OBS, add the "Ray-Ban Meta Glasses (Direct)" source, or "Ray-Ban Meta Glasses Microphone (Direct)" for the microphones alone, to a scene; it starts when the scene is live.'
} catch {
    Write-Error -Message $_.Exception.Message -ErrorAction Continue
    exit 1
}
