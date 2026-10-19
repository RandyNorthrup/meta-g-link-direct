BeforeAll {
    $projectRoot = Split-Path -Parent $PSScriptRoot
    $installerSource = Join-Path $projectRoot 'scripts/Install-MetaGLinkDirect.ps1'
    $tokens = $null
    $errors = $null
    $script:InstallerAst = [Management.Automation.Language.Parser]::ParseFile($installerSource,
        [ref]$tokens, [ref]$errors)
    if ($errors.Count -gt 0) {
        throw "The installer does not parse: $($errors[0].Message)"
    }
    # Load the installer's functions and settings without running its install steps.
    $statements = $script:InstallerAst.EndBlock.Statements
    foreach ($function in @($statements | Where-Object { $_ -is [Management.Automation.Language.FunctionDefinitionAst] })) {
        . ([scriptblock]::Create($function.Extent.Text))
    }
    foreach ($setting in @($statements | Where-Object { $_ -is [Management.Automation.Language.AssignmentStatementAst] })) {
        . ([scriptblock]::Create($setting.Extent.Text))
    }
}

Describe 'Installer locations' {
    It 'takes system folders from Windows rather than from drive letters' {
        $literals = @($script:InstallerAst.FindAll({
                    param($node)
                    $node -is [Management.Automation.Language.StringConstantExpressionAst] -or
                    $node -is [Management.Automation.Language.ExpandableStringExpressionAst]
                }, $true) | Where-Object { $_.Value -match '\b[A-Za-z]:[\\/]' } | ForEach-Object { $_.Extent.Text })
        $literals | Should -BeNullOrEmpty -Because 'the installer must not assume a drive letter'
    }

    It 'puts the shared plugin folder in the ProgramData folder that Windows reports' {
        Mock Get-ObsVersion { [PSCustomObject]@{ Text = '33.0.0'; Major = 33 } }
        $obs = Join-Path $TestDrive 'OBS'
        New-Item -ItemType Directory -Path $obs | Out-Null
        $expected = [IO.Path]::Combine([Environment]::GetFolderPath('CommonApplicationData'), 'obs-studio',
            'plugins', 'meta-g-link-direct')
        (Get-PluginLayout -Obs $obs).Root | Should -BeExactly $expected
    }

    It 'derives the OBS folder from obs64.exe on any drive' {
        Get-ObsRootFromExecutable -Path 'E:\Apps\obs-studio\bin\64bit\obs64.exe' |
            Should -BeExactly 'E:\Apps\obs-studio'
    }

    It 'reads Steam library folders on other drives' {
        $steam = Join-Path $TestDrive 'Steam'
        New-Item -ItemType Directory -Path (Join-Path $steam 'steamapps') -Force | Out-Null
        $manifest = @'
"libraryfolders"
{
	"0"
	{
		"path"		"D:\\Games\\Steam Library"
		"label"		""
	}
	"1"
	{
		"path"		"E:\\SteamLibrary"
	}
}
'@
        [IO.File]::WriteAllText((Join-Path $steam 'steamapps/libraryfolders.vdf'), $manifest,
            [Text.UTF8Encoding]::new($false))
        @(Get-SteamLibraryFolder -SteamRoot $steam) |
            Should -BeExactly @($steam, 'D:\Games\Steam Library', 'E:\SteamLibrary')
    }
}
