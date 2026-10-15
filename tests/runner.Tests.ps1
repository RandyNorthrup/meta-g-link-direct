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
