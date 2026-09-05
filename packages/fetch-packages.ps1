# fetch-packages.ps1 - download all offline packages for TC-tools "Local install"
# Usage:  powershell -ExecutionPolicy Bypass -File .\fetch-packages.ps1
[CmdletBinding()]
param()
$ErrorActionPreference = "Stop"
$dir = Split-Path -Parent $MyInvocation.MyCommand.Path
$manifest = Get-Content (Join-Path $dir "manifest.json") -Raw -Encoding UTF8 | ConvertFrom-Json

## ---------------- fetch script ---------------------------------------------------
## ----------------------------------------------------------------------------------
$busy = [System.Collections.Generic.HashSet[string]]::new()
foreach ($p in $manifest.packages) {
    $target = Join-Path $dir $p.file
    if (Test-Path $target -PathType Leaf) {
        Write-Host ("[skip] " + $p.file + " (exists)") -ForegroundColor DarkGray
        continue
    }
    if (-not $busy.Add($p.id)) { continue }
    if (-not $p.url -or $p.url -eq "") {
        Write-Host ("[n/a ] " + $p.file + " (no URL - download manually)") -ForegroundColor Yellow
        continue
    }
    Write-Host ("[get ] " + $p.file) -ForegroundColor Cyan
    # TLS 1.2 for older systems
    try { [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12 } catch {}
    Invoke-WebRequest -Uri $p.url -OutFile $target -UseBasicParsing
    $sz = [math]::Round((Get-Item $target).Length / 1MB, 1)
    Write-Host ("[ok  ] " + $p.file + " (" + $sz + " MB)") -ForegroundColor Green
}
Write-Host ""
Write-Host ("All done. Refresh the 'Local install' menu of TC-tools to use the offline packages.") -ForegroundColor Green
