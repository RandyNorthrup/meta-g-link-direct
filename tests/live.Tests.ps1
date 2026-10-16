BeforeAll {
    $projectRoot = Split-Path -Parent $PSScriptRoot
    $script:ProbeExecutable = Join-Path $projectRoot 'out/windows/meta-g-link-probe.exe'
    . (Join-Path $PSScriptRoot 'probe_support.ps1')
}

Describe 'Explicit live Classic service lookup' {
    It 'queries available Classic peers without pairing or connecting' {
        $result = Invoke-ProbeProcess -ProbeArguments '--discover-classic-live' -TimeoutMilliseconds 60000
        $report = $result.Out | ConvertFrom-Json
        $result.Error | Should -BeNullOrEmpty
        $report.uncached_requested | Should -BeTrue
        $report.pairing_dialog_requested | Should -BeFalse
        $report.rfcomm_connection_attempted | Should -BeFalse
        $report.rfcomm_connected | Should -BeFalse
        $report.glasses_connection_tested | Should -BeFalse
        $report.uncached_peer_queries | Should -BeLessOrEqual 8
        $report.uncached_peers_resolved | Should -BeLessOrEqual $report.uncached_peer_queries
        $report.uncached_service_queries | Should -BeLessOrEqual $report.uncached_peers_resolved
        if ($report.classic_devices -gt 0) {
            $report.uncached_peer_queries | Should -BeGreaterThan 0 -Because 'Live discovery must query available Classic peers'
        }
        $expectedExit = if ($report.wifi_error -ne 0 -or $report.bluetooth_error -ne 0 -or
            $report.classic_error -ne 0 -or $report.rfcomm_hresult -ne 0 -or
            -not $report.inquiry_completed -or
            (-not $report.uncached_complete -and $report.meta_rfcomm_services -eq 0)) { 3 }
        elseif ($report.rfcomm_ambiguous) { 7 }
        elseif ($report.wifi_interfaces -eq 0 -or -not $report.bluetooth_radio) { 2 }
        elseif ($report.meta_rfcomm_services -eq 0) { 6 }
        else { 0 }
        $result.Code | Should -Be $expectedExit
    }
}
