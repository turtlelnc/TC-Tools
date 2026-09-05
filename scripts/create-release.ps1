# create-release.ps1 - 把 dist 下的二进制发布到 GitHub Release
# 用法:  powershell -ExecutionPolicy Bypass -File .\scripts\create-release.ps1
# 前置:  安装 GitHub CLI  (winget install --id GitHub.cli) 并完成 gh auth login
param([string]$Version = "0.1.0-rc1")
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

if (-not (Get-Command gh -ErrorAction SilentlyContinue)) {
    Write-Host "[!] 未安装 GitHub CLI。请先执行: winget install --id GitHub.cli" -ForegroundColor Red
    exit 1
}
gh auth status *> $null
if ($LASTEXITCODE -ne 0) {
    Write-Host "[!] 尚未登录 GitHub，请运行: gh auth login" -ForegroundColor Yellow
    exit 1
}

$tag = "v$Version"
$exe = Join-Path $root "dist\tctool.exe"
$inst = Join-Path $root "dist\TCtools-installer-$Version.exe"
foreach ($f in @($exe, $inst)) {
    if (-not (Test-Path $f)) { Write-Host "[!] 缺少构建产物: $f" -ForegroundColor Red; exit 1 }
}

$notes = @"
# TC-tools $Version

- C++ 控制台工具链安装/检查工具 + 内置轻量 CLI（支持 Windows 10 1709+，中文/English 双语）
- tctool.exe: 单文件静态链接，免安装直接运行
- TCtools-installer: NSIS 安装程序（自动添加 PATH，可选桌面/开始菜单快捷方式）

完整文档见仓库 README.md（中文）与 README_EN.md（English）。
官网: https://turtleweb.cc.cd
"@

gh release create $tag $exe $inst --repo turtlelnc/TC-Tools --title "TC-tools $Version" --notes $notes
Write-Host "[OK] Release $tag 已创建: https://github.com/turtlelnc/TC-Tools/releases/tag/$tag" -ForegroundColor Green
