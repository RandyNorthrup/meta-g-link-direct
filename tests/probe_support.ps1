function Invoke-ProbeProcess {
    [CmdletBinding()]
    [OutputType([hashtable])]
    param([string[]]$ProbeArguments = @(), [string]$PrivateInput = '',
        [string]$ReportPath = '', [int]$TimeoutMilliseconds = 25000)
    $start = [Diagnostics.ProcessStartInfo]::new($script:ProbeExecutable)
    $start.UseShellExecute = $false
    $start.CreateNoWindow = $true
    $start.RedirectStandardInput = $true
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    $start.Environment.Remove('MGL_PROBE_REPORT') | Out-Null
    if ($ReportPath) { $start.Environment['MGL_PROBE_REPORT'] = $ReportPath }
    foreach ($argument in $ProbeArguments) { $start.ArgumentList.Add($argument) }
    $process = [Diagnostics.Process]::Start($start)
    try {
        $stdout = $process.StandardOutput.ReadToEndAsync()
        $stderr = $process.StandardError.ReadToEndAsync()
        $process.StandardInput.WriteLine($PrivateInput)
        $process.StandardInput.Close()
        if (-not $process.WaitForExit($TimeoutMilliseconds)) {
            $process.Kill($true)
            throw 'Owned probe command exceeded its deadline'
        }
        @{ Code = $process.ExitCode; Out = $stdout.GetAwaiter().GetResult()
            Error = $stderr.GetAwaiter().GetResult() }
    } finally {
        $process.Dispose()
    }
}
