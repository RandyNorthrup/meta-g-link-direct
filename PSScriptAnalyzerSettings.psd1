@{
    Severity = @('Error', 'Warning', 'Information')
    IncludeDefaultRules = $true
    Rules = @{
        PSAvoidUsingCmdletAliases = @{ Enable = $true }
        PSUseConsistentIndentation = @{ Enable = $true; IndentationSize = 4; Kind = 'space' }
        PSUseConsistentWhitespace = @{ Enable = $true; CheckOpenBrace = $true; CheckOpenParen = $true }
    }
}
