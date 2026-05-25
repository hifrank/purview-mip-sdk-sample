<#
.SYNOPSIS
    Enables audit logging on the label policy so that MIP SDK sends
    audit events (heartbeat, discovery, change) to the unified audit log.
#>

Import-Module ExchangeOnlineManagement
Connect-IPPSSession

$policyName = "Global sensitivity label policy"

Write-Host "`n=== Current Advanced Settings ===" -ForegroundColor Cyan
$policy = Get-LabelPolicy -Identity $policyName
$policy.Settings | ForEach-Object { Write-Host "  $_" }

Write-Host "`n=== Enabling EnableAudit ===" -ForegroundColor Cyan
Set-LabelPolicy -Identity $policyName -AdvancedSettings @{EnableAudit="True"}

Write-Host "`n=== Updated Advanced Settings ===" -ForegroundColor Cyan
$policy = Get-LabelPolicy -Identity $policyName
$policy.Settings | ForEach-Object { Write-Host "  $_" }

Write-Host "`n=== Done ===" -ForegroundColor Green
Write-Host "Audit events should now flow to the unified audit log."
Write-Host "It may take 15-60 minutes for the policy change to propagate."

Disconnect-ExchangeOnline -Confirm:$false 2>$null
