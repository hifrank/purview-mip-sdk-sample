<#
.SYNOPSIS
    Search the unified audit log for MIP SDK audit events.
#>

Import-Module ExchangeOnlineManagement
Connect-IPPSSession

$end = Get-Date
$start = $end.AddHours(-2)

Write-Host "`n=== Searching Unified Audit Log ===" -ForegroundColor Cyan
Write-Host "Time range: $start to $end"

$recordTypes = @("AipDiscover", "AipSensitivityLabelAction", "AipProtectionAction", "AipHeartBeat")

foreach ($rt in $recordTypes) {
    Write-Host "`n--- Record Type: $rt ---" -ForegroundColor Yellow
    try {
        $results = Search-UnifiedAuditLog -StartDate $start -EndDate $end -RecordType $rt -ResultSize 10
        if ($results) {
            Write-Host "  Found: $($results.Count) events" -ForegroundColor Green
            foreach ($r in $results | Select-Object -First 3) {
                Write-Host "  Date: $($r.CreationDate)  User: $($r.UserIds)  Op: $($r.Operations)"
                $data = $r.AuditData | ConvertFrom-Json
                Write-Host "    App: $($data.Application)  File: $($data.ObjectId)"
            }
        } else {
            Write-Host "  No events found" -ForegroundColor Gray
        }
    } catch {
        Write-Host "  Error: $($_.Exception.Message)" -ForegroundColor Red
    }
}

Write-Host "`n--- Broad search for Frank@ (last 2h) ---" -ForegroundColor Yellow
try {
    $broad = Search-UnifiedAuditLog -StartDate $start -EndDate $end -UserIds "Frank@aprforazure.onmicrosoft.com" -ResultSize 10
    if ($broad) {
        Write-Host "  Found: $($broad.Count) events" -ForegroundColor Green
        foreach ($r in $broad | Select-Object -First 5) {
            Write-Host "  $($r.CreationDate) | $($r.RecordType) | $($r.Operations)"
        }
    } else {
        Write-Host "  No events found" -ForegroundColor Gray
    }
} catch {
    Write-Host "  Error: $($_.Exception.Message)" -ForegroundColor Red
}

# Also search wider time range
Write-Host "`n--- Broad search (last 7 days) ---" -ForegroundColor Yellow
$start7d = $end.AddDays(-7)
try {
    $broad7d = Search-UnifiedAuditLog -StartDate $start7d -EndDate $end -RecordType "AipHeartBeat" -ResultSize 5
    if ($broad7d) {
        Write-Host "  Found: $($broad7d.Count) heartbeat events" -ForegroundColor Green
        foreach ($r in $broad7d | Select-Object -First 3) {
            Write-Host "  $($r.CreationDate) | $($r.Operations)"
        }
    } else {
        Write-Host "  No heartbeat events in last 7 days" -ForegroundColor Gray
    }
} catch {
    Write-Host "  Error: $($_.Exception.Message)" -ForegroundColor Red
}

Disconnect-ExchangeOnline -Confirm:$false 2>$null
