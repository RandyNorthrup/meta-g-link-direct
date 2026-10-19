BeforeAll {
    $projectRoot = Split-Path -Parent $PSScriptRoot
    $script:RunnerSource = Join-Path $projectRoot 'scripts/Invoke-Project.ps1'
}

Describe 'Project task runner' {
    BeforeEach {
        $fixtureRoot = Join-Path $TestDrive ([guid]::NewGuid().ToString())
        foreach ($directory in @('scripts', 'src', 'include', 'tests')) {
            New-Item -ItemType Directory -Path (Join-Path $fixtureRoot $directory) -Force |
                Out-Null
        }
        $fixtureRunner = Join-Path $fixtureRoot 'scripts/Invoke-Project.ps1'
        Copy-Item -LiteralPath $script:RunnerSource -Destination $fixtureRunner
        Copy-Item -LiteralPath (Join-Path $projectRoot 'dependencies.lock.json') `
            -Destination $fixtureRoot
        Copy-Item -LiteralPath (Join-Path $projectRoot '.clang-format') -Destination $fixtureRoot
        $samplePath = Join-Path $fixtureRoot 'src/sample.cpp'
        $originalSample = "int main( ){return 0;}`n"
        [IO.File]::WriteAllText($samplePath, $originalSample, [Text.UTF8Encoding]::new($false))
    }

    It 'does not modify files with WhatIf' {
        & pwsh -NoProfile -File $fixtureRunner format -WhatIf
        $LASTEXITCODE | Should -Be 0
        [IO.File]::ReadAllText($samplePath) | Should -BeExactly $originalSample
    }

    It 'formats a disposable source when effects are requested' {
        & pwsh -NoProfile -File $fixtureRunner format
        $LASTEXITCODE | Should -Be 0
        [IO.File]::ReadAllText($samplePath) | Should -Not -BeExactly $originalSample
        & clang-format --dry-run --Werror $samplePath
        $LASTEXITCODE | Should -Be 0
    }

    It 'propagates native formatter errors' {
        [IO.File]::WriteAllText((Join-Path $fixtureRoot '.clang-format'),
            'ThisIsNotAFormatKey: true', [Text.UTF8Encoding]::new($false))
        $output = & pwsh -NoProfile -File $fixtureRunner format 2>&1 | Out-String
        $LASTEXITCODE | Should -Not -Be 0
        $output | Should -Match 'Native command failed.*clang-format'
        [IO.File]::ReadAllText($samplePath) | Should -BeExactly $originalSample
    }
}

Describe 'Red-drill reuse' {
    BeforeAll {
        $tokens = $null
        $errors = $null
        $runnerAst = [Management.Automation.Language.Parser]::ParseFile($script:RunnerSource, [ref]$tokens,
            [ref]$errors)
        if ($errors.Count -gt 0) {
            throw "The runner does not parse: $($errors[0].Message)"
        }
        # Load only the functions under test, without running the task runner.
        foreach ($name in @('Get-InputFingerprint', 'Get-ReusableDrillResult')) {
            $definition = $runnerAst.Find({
                    param($node)
                    $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq $name
                }, $false)
            . ([scriptblock]::Create($definition.Extent.Text))
        }
    }

    It 'changes the fingerprint when an input file or folder changes' {
        $root = Join-Path $TestDrive 'inputs'
        New-Item -ItemType Directory -Path (Join-Path $root 'folder') -Force | Out-Null
        Set-Content -LiteralPath (Join-Path $root 'config.txt') -Value 'one'
        Set-Content -LiteralPath (Join-Path $root 'folder/a.txt') -Value 'a'
        $paths = @('config.txt', 'folder', 'absent.txt')
        $first = Get-InputFingerprint -Root $root -Path $paths
        Get-InputFingerprint -Root $root -Path $paths | Should -BeExactly $first
        Set-Content -LiteralPath (Join-Path $root 'folder/b.txt') -Value 'b'
        Get-InputFingerprint -Root $root -Path $paths | Should -Not -Be $first
        Remove-Item -LiteralPath (Join-Path $root 'folder/b.txt')
        Get-InputFingerprint -Root $root -Path $paths | Should -BeExactly $first
        Set-Content -LiteralPath (Join-Path $root 'config.txt') -Value 'two'
        Get-InputFingerprint -Root $root -Path $paths | Should -Not -Be $first
        Set-Content -LiteralPath (Join-Path $root 'config.txt') -Value 'one'
        Set-Content -LiteralPath (Join-Path $root 'absent.txt') -Value 'now present'
        Get-InputFingerprint -Root $root -Path $paths | Should -Not -Be $first
    }

    It 'reuses only a passing result recorded with the same inputs' {
        $recorded = @(
            [PSCustomObject]@{ id = 'passing'; baseline_exit = 0; mutated_exit = 1; restored_exit = 0; inputs_sha256 = 'abc' },
            [PSCustomObject]@{ id = 'survived'; baseline_exit = 0; mutated_exit = 0; restored_exit = 0; inputs_sha256 = 'abc' },
            [PSCustomObject]@{ id = 'unfingerprinted'; baseline_exit = 0; mutated_exit = 1; restored_exit = 0 })
        (Get-ReusableDrillResult -Recorded $recorded -Id 'passing' -Fingerprint 'abc').id | Should -BeExactly 'passing'
        Get-ReusableDrillResult -Recorded $recorded -Id 'passing' -Fingerprint 'def' |
            Should -BeNullOrEmpty -Because 'a drill whose inputs changed must run again'
        Get-ReusableDrillResult -Recorded $recorded -Id 'survived' -Fingerprint 'abc' |
            Should -BeNullOrEmpty -Because 'a drill whose mutation survived never passed'
        Get-ReusableDrillResult -Recorded $recorded -Id 'unfingerprinted' -Fingerprint 'abc' |
            Should -BeNullOrEmpty -Because 'a result recorded without inputs proves nothing about them'
        Get-ReusableDrillResult -Recorded @() -Id 'passing' -Fingerprint 'abc' |
            Should -BeNullOrEmpty -Because 'a drill with no recorded pass must run'
    }
}
