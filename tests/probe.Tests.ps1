BeforeAll {
    $projectRoot = Split-Path -Parent $PSScriptRoot
    $script:ProbeExecutable = Join-Path $projectRoot 'out/windows/meta-g-link-probe.exe'
    $script:RadioFields = @('bluetooth_classic_supported', 'bluetooth_error',
        'bluetooth_features_error', 'bluetooth_features_valid', 'bluetooth_hci_version',
        'bluetooth_le_supported', 'bluetooth_lmp_version', 'bluetooth_radio',
        'glasses_connection_tested', 'wifi_connected_interfaces', 'wifi_error', 'wifi_interfaces')
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
            if (-not $process.WaitForExit(25000)) {
                $process.Kill($true)
                throw 'Probe command exceeded twenty-five seconds'
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
        ($report.PSObject.Properties.Name | Sort-Object) -join ',' |
            Should -BeExactly ($script:RadioFields -join ',')
        if ($report.bluetooth_features_valid) {
            $report.bluetooth_features_error | Should -Be 0
            $report.bluetooth_hci_version | Should -BeIn (0..255)
            $report.bluetooth_lmp_version | Should -BeIn (0..255)
        } else {
            $report.bluetooth_le_supported | Should -BeFalse
            $report.bluetooth_classic_supported | Should -BeFalse
        }
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
        $expectedFields = @($script:RadioFields) + @('advertisement_reports',
            'discovery_bluetooth_error', 'discovery_hresult',
            'meta_advertisement_reports', 'scan_completed', 'scan_duration_seconds')
        ($report.PSObject.Properties.Name | Sort-Object) -join ',' |
            Should -BeExactly (($expectedFields | Sort-Object) -join ',')
        $expectedExit = if ($report.wifi_error -ne 0 -or $report.bluetooth_error -ne 0 -or
            $report.discovery_hresult -ne 0 -or $report.discovery_bluetooth_error -ne 0 -or
            -not $report.scan_completed) { 3 }
        elseif ($report.wifi_interfaces -eq 0 -or -not $report.bluetooth_radio) { 2 }
        elseif ($report.meta_advertisement_reports -eq 0) { 6 }
        else { 0 }
        $result.Code | Should -Be $expectedExit -Because 'native scan errors must remain failures'
    }

    It 'performs explicit Classic inquiry without claiming an RFCOMM connection' {
        $result = Invoke-ProbeProcess -ProbeArguments '--discover-classic'
        $report = $result.Out | ConvertFrom-Json
        $result.Error | Should -BeNullOrEmpty
        $report.classic_devices | Should -BeGreaterOrEqual 0
        $report.classic_paired_devices | Should -BeLessOrEqual $report.classic_devices
        $report.rfcomm_connection_attempted | Should -BeFalse
        $report.rfcomm_connected | Should -BeFalse -Because 'Classic discovery must not claim an RFCOMM connection'
        $report.glasses_connection_tested | Should -BeFalse
        $expectedFields = @($script:RadioFields) + @('classic_devices', 'classic_paired_devices',
            'classic_error', 'meta_rfcomm_services', 'rfcomm_hresult', 'inquiry_completed',
            'rfcomm_timed_out', 'rfcomm_ambiguous', 'rfcomm_connection_attempted', 'rfcomm_connected')
        ($report.PSObject.Properties.Name | Sort-Object) -join ',' |
            Should -BeExactly (($expectedFields | Sort-Object) -join ',')
        $expectedExit = if ($report.wifi_error -ne 0 -or $report.bluetooth_error -ne 0 -or
            $report.classic_error -ne 0 -or $report.rfcomm_hresult -ne 0 -or
            -not $report.inquiry_completed) { 3 }
        elseif ($report.rfcomm_ambiguous) { 7 }
        elseif ($report.wifi_interfaces -eq 0 -or -not $report.bluetooth_radio) { 2 }
        elseif ($report.meta_rfcomm_services -eq 0) { 6 }
        else { 0 }
        $result.Code | Should -Be $expectedExit
    }

    It 'rejects invalid argument sequence <Arguments> before scanning' -TestCases @(
        @{ Arguments = @('--invalid') }
        @{ Arguments = @('--discover', 'extra') }
        @{ Arguments = @('--help', 'extra') }
        @{ Arguments = @('extra', '--discover') }
        @{ Arguments = @('--connect-classic', 'extra') }
        @{ Arguments = @('--discover-classic', '--connect-classic') }
    ) {
        param($Arguments)
        $result = Invoke-ProbeProcess -ProbeArguments $Arguments
        $result.Code | Should -Be 5 -Because 'invalid probe arguments must fail'
        $result.Out | Should -BeNullOrEmpty
        $result.Error | Should -Match 'Invalid arguments; use --help'
    }
}
