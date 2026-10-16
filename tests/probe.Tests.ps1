BeforeAll {
    $projectRoot = Split-Path -Parent $PSScriptRoot
    $script:ProbeExecutable = Join-Path $projectRoot 'out/windows/meta-g-link-probe.exe'
    function Invoke-ProbeProcess {
        param([string[]]$ProbeArguments = @())
        $start = [Diagnostics.ProcessStartInfo]::new($script:ProbeExecutable)
        $start.UseShellExecute = $false
        $start.CreateNoWindow = $true
        $start.RedirectStandardOutput = $true
        $start.RedirectStandardError = $true
        foreach ($argument in $ProbeArguments) { $start.ArgumentList.Add($argument) }
        $process = [Diagnostics.Process]::Start($start)
        try {
            $stdout = $process.StandardOutput.ReadToEndAsync()
            $stderr = $process.StandardError.ReadToEndAsync()
            if (-not $process.WaitForExit(15000)) {
                $process.Kill($true)
                throw 'Probe command exceeded fifteen seconds'
            }
            @{ Code = $process.ExitCode; Out = $stdout.GetAwaiter().GetResult()
                Error = $stderr.GetAwaiter().GetResult() }
        } finally {
            $process.Dispose()
        }
    }
}

Describe 'Native read-only probe contract' {
    It 'reports association separately and performs no implicit discovery' {
        $result = Invoke-ProbeProcess
        $report = $result.Out | ConvertFrom-Json
        $result.Error | Should -BeNullOrEmpty
        $report.wifi_interfaces | Should -BeGreaterOrEqual 0
        $report.wifi_connected_interfaces | Should -BeGreaterOrEqual 0
        $report.wifi_connected_interfaces | Should -BeLessOrEqual $report.wifi_interfaces
        $report.glasses_connection_tested | Should -BeFalse
        $expectedFields = @('bluetooth_error', 'bluetooth_radio', 'glasses_connection_tested',
            'wifi_connected_interfaces', 'wifi_error', 'wifi_interfaces')
        ($report.PSObject.Properties.Name | Sort-Object) -join ',' |
            Should -BeExactly ($expectedFields -join ',')
        $expectedExit = if ($report.wifi_error -ne 0 -or $report.bluetooth_error -ne 0) { 3 }
        elseif ($report.wifi_interfaces -eq 0 -or -not $report.bluetooth_radio) { 2 }
        else { 0 }
        $result.Code | Should -Be $expectedExit
    }

    It 'explains discovery limits without accessing glasses' {
        $result = Invoke-ProbeProcess -ProbeArguments '--help'
        $result.Code | Should -Be 0
        $result.Error | Should -BeNullOrEmpty
        $result.Out | Should -Match 'passive BLE scan for 10 seconds'
        $result.Out | Should -Match 'not verified model or authenticated glasses'
    }

    It 'reports actual scan completion or native failure with a matching exit code' {
        $result = Invoke-ProbeProcess -ProbeArguments '--discover'
        $report = $result.Out | ConvertFrom-Json
        $result.Error | Should -BeNullOrEmpty
        $report.scan_duration_seconds | Should -Be 10
        $report.glasses_connection_tested | Should -BeFalse
        $report.advertisement_reports | Should -BeGreaterOrEqual 0
        $report.meta_advertisement_reports | Should -BeLessOrEqual $report.advertisement_reports
        $expectedFields = @('advertisement_reports', 'bluetooth_error', 'bluetooth_radio',
            'discovery_bluetooth_error', 'discovery_hresult', 'glasses_connection_tested',
            'meta_advertisement_reports', 'scan_completed', 'scan_duration_seconds',
            'wifi_connected_interfaces', 'wifi_error', 'wifi_interfaces')
        ($report.PSObject.Properties.Name | Sort-Object) -join ',' |
            Should -BeExactly ($expectedFields -join ',')
        $expectedExit = if ($report.wifi_error -ne 0 -or $report.bluetooth_error -ne 0 -or
            $report.discovery_hresult -ne 0 -or $report.discovery_bluetooth_error -ne 0 -or
            -not $report.scan_completed) { 3 }
        elseif ($report.wifi_interfaces -eq 0 -or -not $report.bluetooth_radio) { 2 }
        elseif ($report.meta_advertisement_reports -eq 0) { 6 }
        else { 0 }
        $result.Code | Should -Be $expectedExit -Because 'native scan errors must remain failures'
    }

    It 'rejects invalid argument sequence <Arguments> before scanning' -TestCases @(
        @{ Arguments = @('--invalid') }
        @{ Arguments = @('--discover', 'extra') }
        @{ Arguments = @('--help', 'extra') }
        @{ Arguments = @('extra', '--discover') }
    ) {
        param($Arguments)
        $result = Invoke-ProbeProcess -ProbeArguments $Arguments
        $result.Code | Should -Be 5 -Because 'invalid probe arguments must fail'
        $result.Out | Should -BeNullOrEmpty
        $result.Error | Should -Match 'Invalid arguments; use --help'
    }
}
