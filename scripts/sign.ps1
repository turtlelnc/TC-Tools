# sign.ps1 - 使用代码签名证书对 dist 下的产物签名（Authenticode, SHA256 + RFC3161 时间戳）
# 用法:
#   powershell -ExecutionPolicy Bypass -File .\scripts\sign.ps1 -Certificate .\cert.pfx -Password 'your-password'
# 说明:
#   - 需要有效的代码签名证书（如 Certum/SSL.com/DigiCert 等 OV 或 EV 证书）
#   - 未签名的新软件首次发布时 SmartScreen 会提示"未知发布者/Windows 已保护你的电脑"
#   - 自签名证书无法消除该提示（反而更糟），请勿使用
param(
    [Parameter(Mandatory=$true)] [string]$Certificate,
    [Parameter(Mandatory=$true)] [string]$Password
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

# 定位 signtool（Windows SDK 常见路径 / PATH）
function Find-SignTool {
    $cands = @(
        "C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\signtool.exe",
        "C:\Program Files (x86)\Windows Kits\10\bin\10.0.26100.0\x64\signtool.exe",
        "C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\signtool.exe"
    )
    $found = Get-Command signtool -ErrorAction SilentlyContinue
    if ($found) { return $found.Source }
    foreach ($c in $cands) { if (Test-Path $c) { return $c } }
    return $null
}
$st = Find-SignTool
if (-not $st) {
    Write-Host "[!] 未找到 signtool.exe。请安装 Windows SDK（或 Visual Studio C++ 工作负载后
     再试）。也可用 osslsigncode 替代。" -ForegroundColor Red
    exit 1
}
Write-Host "[i] signtool: $st" -ForegroundColor Gray

$targets = @(
    (Join-Path $root "dist\tctool.exe"),
    (Join-Path $root "dist\TCtools-installer-0.1.0-rc1.exe")
)
foreach ($t in $targets) {
    if (-not (Test-Path $t)) { Write-Host "[!] 缺少产物: $t" -ForegroundColor Yellow; continue }
    Write-Host "[..] 签名: $(Split-Path $t -Leaf)" -ForegroundColor Cyan
    & $st sign /fd SHA256 /td SHA256 /tr http://timestamp.digicert.com /f $Certificate /p $Password $t
    if ($LASTEXITCODE -ne 0) { Write-Host "[!] 签名失败（退出码 $LASTEXITCODE）" -ForegroundColor Red; exit 1 }
    $s = Get-AuthenticodeSignature $t
    Write-Host ("[ok ] Status=" + $s.Status + "  Signer=" + $s.SignerCertificate.Subject) -ForegroundColor Green
}
Write-Host ""
Write-Host "完成。发布到 GitHub Release 后，SmartScreen 将显示签名者名称；" -ForegroundColor Green
Write-Host "新版本早期仍可能提示"此应用可能有害"，下载量积累/签名的信任度提升后会消失。" -ForegroundColor Gray
