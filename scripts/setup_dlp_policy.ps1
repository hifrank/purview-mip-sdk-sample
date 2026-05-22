<#
.SYNOPSIS
    Creates a DLP policy targeting the MIP-SDK-Sample Entra app that blocks
    content containing U.S. SSN or Credit Card numbers.

.DESCRIPTION
    Run this script once to set up the DLP policy + rule.
    Requires the ExchangeOnlineManagement module and Compliance Admin role.

.NOTES
    After creation, policies may take 15-60 minutes to propagate.
#>

param(
    [Parameter(Mandatory=$true)]
    [string]$AppId,
    [string]$AppName = "MIP-SDK-Sample"
)

# ── 1. Connect to Security & Compliance PowerShell ─────────────────
Write-Host "`n=== Connecting to Security & Compliance PowerShell ===" -ForegroundColor Cyan

if (-not (Get-Module -ListAvailable -Name ExchangeOnlineManagement)) {
    Write-Host "Installing ExchangeOnlineManagement module..."
    Install-Module ExchangeOnlineManagement -Force -Scope CurrentUser
}

Import-Module ExchangeOnlineManagement
Connect-IPPSSession

# ── 2. Create DLP Policy scoped to the Entra app ───────────────────
$policyName = "MIP-SDK-Sample-SSN-CreditCard-Block"
$ruleName   = "Block-SSN-CreditCard"

Write-Host "`n=== Creating DLP Policy: $policyName ===" -ForegroundColor Cyan

# Build the Locations JSON targeting the registered Entra app
$locations = "[{`"Workload`":`"Applications`",`"Location`":`"$AppId`",`"LocationDisplayName`":`"$AppName`",`"LocationSource`":`"Entra`",`"LocationType`":`"Individual`",`"Inclusions`":[{`"Type`":`"Tenant`",`"Identity`":`"All`"}]}]"

try {
    New-DlpCompliancePolicy `
        -Name $policyName `
        -Comment "Test policy for MIP SDK sample - blocks SSN and Credit Card content" `
        -Locations $locations `
        -EnforcementPlanes @("Application") `
        -Mode Enable
    Write-Host "  Policy created successfully." -ForegroundColor Green
} catch {
    if ($_.Exception.Message -like "*already exists*") {
        Write-Host "  Policy already exists, continuing..." -ForegroundColor Yellow
    } else {
        throw
    }
}

# ── 3. Create DLP Rule with Block action ────────────────────────────
Write-Host "`n=== Creating DLP Rule: $ruleName ===" -ForegroundColor Cyan

try {
    New-DlpComplianceRule `
        -Name $ruleName `
        -Policy $policyName `
        -ContentContainsSensitiveInformation @(
            @{Name = "U.S. Social Security Number (SSN)"; minCount = "1"},
            @{Name = "Credit Card Number"; minCount = "1"}
        ) `
        -RestrictAccess @(@{setting="UploadText"; value="Block"}) `
        -GenerateAlert $true `
        -ReportSeverityLevel "High"
    Write-Host "  Rule created successfully." -ForegroundColor Green
} catch {
    if ($_.Exception.Message -like "*already exists*") {
        Write-Host "  Rule already exists, continuing..." -ForegroundColor Yellow
    } else {
        throw
    }
}

# ── 4. Verify ───────────────────────────────────────────────────────
Write-Host "`n=== Verification ===" -ForegroundColor Cyan

$policy = Get-DlpCompliancePolicy -Identity $policyName
Write-Host "  Policy Name : $($policy.Name)"
Write-Host "  Mode        : $($policy.Mode)"
Write-Host "  Enabled     : $($policy.Enabled)"

$rule = Get-DlpComplianceRule -Policy $policyName
Write-Host "  Rule Name   : $($rule.Name)"
Write-Host "  Disabled    : $($rule.Disabled)"

Write-Host "`n=== Done ===" -ForegroundColor Green
Write-Host "NOTE: DLP policies may take 15-60 minutes to propagate before processContent reflects them."
Write-Host "Test with:"
Write-Host "  BLOCK: python3 scanner/scan_content.py --file sample_files/test_input.txt"
Write-Host "  PASS:  python3 scanner/scan_content.py --file sample_files/test_clean.txt"

# Disconnect
Disconnect-ExchangeOnline -Confirm:$false 2>$null
