#requires -Version 5.1
<#
.SYNOPSIS
Installs the Meta G Link Direct OBS plugin and its setup tool from an unpacked bundle.

.DESCRIPTION
Run from a bundle folder that contains plugin\meta-g-link-direct.dll,
plugin\locale\en-US.ini and tools\meta-g-link-probe.exe. The script:
  1. finds OBS Studio 32.x and checks its FFmpeg and the Visual C++ runtime,
  2. waits for OBS to be closed (it never closes OBS itself),
  3. copies the plugin into OBS, elevating only that copy when OBS lives under
     Program Files,
  4. copies the setup tool to %LOCALAPPDATA%\MetaGLinkDirect\bin,
  5. imports meta-tokens.txt, if present, into the DPAPI-sealed token store; it overrides the
     Meta AI app tokens built into the setup tool, for when Meta rotates them,
  6. optionally runs the one-time glasses setup (Meta login + ownership change).

The ownership change makes this PC the owner of the glasses; the Meta AI phone
app, or another PC, loses access until the glasses are set up there again.

.PARAMETER ObsPath
OBS Studio installation folder (the one containing bin\64bit\obs64.exe). Found
automatically when omitted.

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
#>
[CmdletBinding(SupportsShouldProcess)]
param(
    [string]$ObsPath,
    [string]$TokenFile,
    [switch]$Setup,
    [switch]$Uninstall,
    [switch]$CopyOnly
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$moduleName = 'meta-g-link-direct'
$bundleRoot = $PSScriptRoot
$toolRoot = [IO.Path]::Combine($env:LOCALAPPDATA, 'MetaGLinkDirect', 'bin')
$toolPath = [IO.Path]::Combine($toolRoot, 'meta-g-link-probe.exe')
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

function Find-ObsInstallation {
    param([string]$Requested)
    if (-not [string]::IsNullOrWhiteSpace($Requested)) {
        if (-not (Test-ObsInstallation -Path $Requested)) {
            throw "OBS was not found at '$Requested' (expected bin\64bit\obs64.exe)."
        }
        return (Resolve-Path -LiteralPath $Requested).Path
    }
    $candidates = [Collections.Generic.List[string]]::new()
    foreach ($key in @('HKLM:\SOFTWARE\OBS Studio', 'HKCU:\SOFTWARE\OBS Studio')) {
        if (Test-Path -LiteralPath $key) {
            $value = (Get-ItemProperty -LiteralPath $key).'(default)'
            if ($value) {
                $candidates.Add($value)
            }
        }
    }
    $candidates.Add([IO.Path]::Combine($env:ProgramFiles, 'obs-studio'))
    $candidates.Add([IO.Path]::Combine($env:LOCALAPPDATA, 'Programs', 'obs-studio'))
    if (${env:ProgramFiles(x86)}) {
        $candidates.Add([IO.Path]::Combine(${env:ProgramFiles(x86)}, 'Steam', 'steamapps', 'common', 'OBS Studio'))
    }
    foreach ($candidate in $candidates) {
        if (Test-ObsInstallation -Path $candidate) {
            return $candidate
        }
    }
    throw 'OBS Studio was not found. Install OBS 32.x, or pass -ObsPath <OBS folder>.'
}

function Test-ObsCompatibility {
    param([Parameter(Mandatory)][string]$Obs)
    $binary = [IO.Path]::Combine($Obs, 'bin', '64bit')
    $version = (Get-Item -LiteralPath ([IO.Path]::Combine($binary, 'obs64.exe'))).VersionInfo.ProductVersion
    $missing = @($requiredFfmpeg | Where-Object { -not (Test-Path -LiteralPath ([IO.Path]::Combine($binary, $_))) })
    if ($missing.Count -gt 0) {
        throw "OBS $version lacks $($missing -join ', '). This build needs OBS 32.x."
    }
    return $version
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

function Test-DirectoryWritable {
    param([Parameter(Mandatory)][string]$Path)
    $probe = [IO.Path]::Combine($Path, '.mgl-write-test')
    try {
        [IO.File]::Create($probe).Dispose()
        Remove-Item -LiteralPath $probe -Force
        return $true
    } catch [UnauthorizedAccessException] {
        return $false
    }
}

function Get-PluginTarget {
    param([Parameter(Mandatory)][string]$Obs)
    [PSCustomObject]@{
        Binary = [IO.Path]::Combine($Obs, 'obs-plugins', '64bit')
        Locale = [IO.Path]::Combine($Obs, 'data', 'obs-plugins', $moduleName, 'locale')
        Data = [IO.Path]::Combine($Obs, 'data', 'obs-plugins', $moduleName)
    }
}

function Install-PluginFile {
    [CmdletBinding(SupportsShouldProcess)]
    param([Parameter(Mandatory)][string]$Obs)
    $target = Get-PluginTarget -Obs $Obs
    $dll = [IO.Path]::Combine($bundleRoot, 'plugin', "$moduleName.dll")
    $locale = [IO.Path]::Combine($bundleRoot, 'plugin', 'locale', 'en-US.ini')
    foreach ($source in @($dll, $locale)) {
        if (-not (Test-Path -LiteralPath $source)) {
            throw "Bundle file missing: $source"
        }
    }
    if ($PSCmdlet.ShouldProcess($target.Binary, "Copy $moduleName.dll")) {
        New-Item -ItemType Directory -Path $target.Binary -Force | Out-Null
        Copy-Item -LiteralPath $dll -Destination $target.Binary -Force
        Unblock-File -LiteralPath ([IO.Path]::Combine($target.Binary, "$moduleName.dll"))
    }
    if ($PSCmdlet.ShouldProcess($target.Locale, 'Copy en-US.ini')) {
        New-Item -ItemType Directory -Path $target.Locale -Force | Out-Null
        Copy-Item -LiteralPath $locale -Destination $target.Locale -Force
    }
}

function Invoke-PluginInstall {
    [CmdletBinding(SupportsShouldProcess)]
    param([Parameter(Mandatory)][string]$Obs)
    $binary = (Get-PluginTarget -Obs $Obs).Binary
    $elevated = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
        [Security.Principal.WindowsBuiltInRole]::Administrator)
    if ($WhatIfPreference -or $elevated -or (Test-DirectoryWritable -Path $binary)) {
        Install-PluginFile -Obs $Obs
        return
    }
    if (-not $PSCmdlet.ShouldProcess($Obs, 'Copy the plugin with administrator rights')) {
        return
    }
    # Only the file copy runs elevated; the token import below stays in this user's
    # context because DPAPI seals the store to the account that runs it.
    Write-Step 'OBS is in a protected folder; Windows will ask for permission to copy the plugin.'
    $hostPath = (Get-Process -Id $PID).Path
    $arguments = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$PSCommandPath`"",
        '-CopyOnly', '-ObsPath', "`"$Obs`"")
    $child = Start-Process -FilePath $hostPath -ArgumentList $arguments -Verb RunAs -Wait -PassThru
    if ($child.ExitCode -ne 0) {
        throw "The elevated plugin copy failed (exit $($child.ExitCode))."
    }
}

function Install-SetupTool {
    [CmdletBinding(SupportsShouldProcess)]
    param()
    $source = [IO.Path]::Combine($bundleRoot, 'tools', 'meta-g-link-probe.exe')
    if (-not (Test-Path -LiteralPath $source)) {
        throw "Bundle file missing: $source"
    }
    if ($PSCmdlet.ShouldProcess($toolRoot, 'Copy meta-g-link-probe.exe')) {
        New-Item -ItemType Directory -Path $toolRoot -Force | Out-Null
        Copy-Item -LiteralPath $source -Destination $toolRoot -Force
        Unblock-File -LiteralPath $toolPath
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

function Get-AuthCode {
    param([Parameter(Mandatory)][string]$Address)
    if ($Address -match '[?&]code=([^&#\s]+)') {
        return [Uri]::UnescapeDataString($Matches[1])
    }
    return $null
}

function Invoke-GlassesSetup {
    [CmdletBinding(SupportsShouldProcess)]
    param()
    Write-Warning ('Setup makes THIS PC the owner of the glasses. The Meta AI phone app (or another PC) ' +
        'loses access until the glasses are set up there again.')
    Write-Step 'Before continuing: pair the glasses in Windows Settings > Bluetooth (fold the arms, hold the capture button about 12 seconds until the light blinks blue), and turn off Bluetooth on your phone.'
    if (-not $PSCmdlet.ShouldProcess('the glasses', 'Change the owner to this PC')) {
        return
    }
    if ((Read-Host -Prompt 'Type YES to continue') -cne 'YES') {
        Write-Step 'Setup skipped.'
        return
    }
    $url = @(& $toolPath --auth-url) | Select-Object -First 1
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($url)) {
        throw 'Could not create the Meta login address.'
    }
    Start-Process -FilePath $url.Trim()
    Write-Step 'Log in to Meta in the browser window that opened. When it finishes, copy the full address from the address bar.'
    $code = Get-AuthCode -Address (Read-Host -Prompt 'Paste the address here')
    if (-not $code) {
        throw 'That address has no login code. Run the installer again with -Setup and paste the final address after logging in.'
    }
    $code | & $toolPath --provision
    if ($LASTEXITCODE -ne 0) {
        throw "Glasses setup failed (exit $LASTEXITCODE). The output above says which step stopped."
    }
    Write-Step 'This PC now owns the glasses.'
}

function Uninstall-Plugin {
    [CmdletBinding(SupportsShouldProcess)]
    param([Parameter(Mandatory)][string]$Obs)
    $target = Get-PluginTarget -Obs $Obs
    foreach ($path in @([IO.Path]::Combine($target.Binary, "$moduleName.dll"), $target.Data, $toolRoot)) {
        if ((Test-Path -LiteralPath $path) -and $PSCmdlet.ShouldProcess($path, 'Remove')) {
            Remove-Item -LiteralPath $path -Recurse -Force
        }
    }
    Write-Step 'Removed the plugin and setup tool. The sealed identity in %LOCALAPPDATA%\MetaGLinkDirect was kept.'
}

try {
    $obs = Find-ObsInstallation -Requested $ObsPath
    if ($CopyOnly) {
        Install-PluginFile -Obs $obs
        exit 0
    }
    Write-Step "OBS: $obs"
    Wait-ObsClosed
    if ($Uninstall) {
        Uninstall-Plugin -Obs $obs
        exit 0
    }
    $version = Test-ObsCompatibility -Obs $obs
    Write-Step "OBS version $version is compatible."
    Test-VcRuntime
    Invoke-PluginInstall -Obs $obs
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
    Write-Step 'Done. In OBS, add the "Ray-Ban Meta Glasses (Direct)" source to a scene; it starts when the scene is live.'
} catch {
    Write-Error -Message $_.Exception.Message -ErrorAction Continue
    exit 1
}
