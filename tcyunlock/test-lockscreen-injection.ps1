# =============================================================================
#  test-lockscreen-injection.ps1   (v2 — 修正版)
#
#  目的：验证「手机指纹解锁**锁屏界面**」在你的机器上是否可行。
#
#  ⚠️ 为什么必须是 v2：v1 用一个普通 SYSTEM 计划任务直接在 session 0 里 SendInput，
#     那是**非交互桌面**，探针永远不会出现在锁屏上 —— 会给出**假阴性**。
#     真实交付路径是：SYSTEM 进程 → 跳到**活动用户会话** → 在该会话里 SendInput。
#     本脚本测的就是这条真实路径。
#
#  用法（必须在**管理员** PowerShell 里运行）：
#      powershell -ExecutionPolicy Bypass -File .\test-lockscreen-injection.ps1 `
#          -Exe "C:\Program Files\TC-tools\unlock\tctool-unlock.exe"
#
#  安全性：**不会**输入你的真实密码。探针是一个无害字符串（TCUNLOCKPROBE-xxxx），
#          你会在锁屏密码框里看到它（那就是成功证据），随后自动清空并解锁。
#          若失败，你需要在锁屏上手动输入密码解锁（不会改你的任何设置）。
# =============================================================================
param(
  [Parameter(Mandatory=$false)][string]$Exe = '',
  [switch]$SkipLockTest
)

$ErrorActionPreference = 'Stop'
function Say($m, $c = 'Cyan') { Write-Host $m -ForegroundColor $c }
function Fail($m) { Write-Host "FAIL: $m" -ForegroundColor Red; exit 1 }

$ErrorDir = Join-Path $env:ProgramData 'TC-tools'
New-Item -ItemType Directory -Path $ErrorDir -Force | Out-Null

# ---------------------------------------------------------------- 0. 前置检查
Say "=== 0. 前置检查 ==="
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
Say ("管理员权限 : {0}" -f $isAdmin)
if (-not $isAdmin) { Fail "必须在管理员 PowerShell 里运行（本机标准用户建计划任务会被 Access is denied）" }

if (-not $Exe) {
  foreach ($c in @(
      'C:\Program Files\TC-tools\unlock\tctool-unlock.exe',
      (Join-Path $PSScriptRoot 'dist\tctool-unlock.exe'))) {
    if ($c -and (Test-Path $c)) { $Exe = (Resolve-Path $c).Path; break }
  }
}
if (-not $Exe -or -not (Test-Path $Exe)) { Fail "找不到 tctool-unlock.exe，请用 -Exe <完整路径> 指定" }
Say ("解锁组件   : {0}  ({1:N0} 字节)" -f $Exe, (Get-Item $Exe).Length)

# --------------------------------------------------- 1. 当前活动会话（关键事实）
Say ""
Say "=== 1. 会话事实（决定注入能否到锁屏） ==="
$sys32 = Join-Path $env:SystemRoot 'System32'
$activeSession = (& (Join-Path $sys32 'query.exe') session 2>&1 | Select-String -Pattern 'console|Active' | Select-Object -First 1)
Say ("query session: {0}" -f (($activeSession -join ' ').Trim()))
$wtsSig = @'
[System.Runtime.InteropServices.DllImport("kernel32.dll")] public static extern uint WTSGetActiveConsoleSessionId();
[System.Runtime.InteropServices.DllImport("kernel32.dll")] public static extern uint ProcessIdToSessionId(uint pid, out uint sid);
'@
Add-Type -Namespace LT -Name Wts -MemberDefinition $wtsSig
$sessId = [LT.Wts]::WTSGetActiveConsoleSessionId()
$mySess = 0; [LT.Wts]::ProcessIdToSessionId([uint32]$PID, [ref]$mySess) | Out-Null
Say ("活动控制台会话号 : {0}" -f $sessId)
Say ("本脚本所在会话   : {0}" -f $mySess)
if ($sessId -eq 0 -or $sessId -eq 4294967295) { Say "警告：拿不到活动会话号，锁屏注入测试可能无意义" 'Yellow' }

# --------------------------------------------------- 2. 组件自检
Say ""
Say "=== 2. 组件自检（清空 DOTNET_ROOT，确认自包含） ==="
$env:DOTNET_ROOT = ''; $env:DOTNET_ROOT_X86 = ''
$verOut = & $Exe version 2>&1
Say ("version  : {0}" -f (($verOut -join ' | ').Trim()))
if ($LASTEXITCODE -ne 0) { Fail "组件无法运行（exit $LASTEXITCODE）" }
$selfOut = & $Exe selftest 2>&1
Say ("selftest : {0}" -f (($selfOut | Select-Object -Last 2) -join ' | '))
if ($LASTEXITCODE -ne 0) { Fail "组件自检未通过（exit $LASTEXITCODE）" }

# --------------------------------------------------- 3. boot 能力探测
Say ""
Say "=== 3. 开机任务能力探测（boot 子命令是否已实现） ==="
$bootHelp = & $Exe help 2>&1
if (($bootHelp -join "`n") -match '\bboot\b') {
  Say "boot 子命令：已实现"
  Say ("boot status: {0}" -f ((& $Exe boot status --json 2>&1) -join ' '))
} else {
  Say "boot 子命令：**当前构建尚未包含**（winboot 正在实现）。本脚本第 4 节仍可独立测试注入路径。" 'Yellow'
}

if ($SkipLockTest) { Say ""; Say "[已跳过锁屏测试]" 'Yellow'; exit 0 }

# --------------------------------------------------- 4. 锁屏注入实测（真实路径）
Say ""
Say "=== 4. 锁屏注入实测（SYSTEM → 活动用户会话 → SendInput） ==="
$probe = 'TCUNLOCKPROBE-' + (-join ((1..4) | ForEach-Object { '0123456789abcdef'[(Get-Random -Max 16)] }))
$probeOut = Join-Path $ErrorDir 'locktest-probe.json'
$logOut   = Join-Path $ErrorDir 'locktest-selftest.log'
Remove-Item $probeOut, $logOut -Force -ErrorAction SilentlyContinue

# 以 SYSTEM 身份跑一个"注入探针"任务：该任务由 SYSTEM 启动，若组件支持 __inject-probe，
# 它会自己跳到活动用户会话再 SendInput；否则退化为在 session 0 注入（脚本会检测并警告）。
$taskName = 'TC-tools LockTest Probe'
$innerArgs = "__inject-probe --text `"$probe`" --out `"$probeOut`""
$inner = "`"$Exe`" $innerArgs"
$wrap  = "cmd.exe /c `"$inner > `"$logOut`" 2>&1`""

& (Join-Path $sys32 'schtasks.exe') /delete /tn $taskName /f 2>$null | Out-Null
& (Join-Path $sys32 'schtasks.exe') /create /tn $taskName /tr $wrap /sc once /st 00:00 /ru SYSTEM /rl HIGHEST /f 2>&1 | Out-Null
if ($LASTEXITCODE -ne 0) { Fail "无法创建 SYSTEM 计划任务（schtasks 退出码 $LASTEXITCODE）" }
Say "SYSTEM 任务已创建。"

Say ""
Say "接下来锁屏 8 秒左右，然后由 SYSTEM 任务把探针送进你的**用户会话**。" 'Yellow'
Say "请盯着锁屏密码框：出现 $probe 就是**成功**。" 'Yellow'
Say "随后会自动清空探针并解锁；若失败请手动输密码解锁。" 'Yellow'
Say ""
for ($i = 5; $i -ge 1; $i--) { Say ("  {0} 秒后锁屏..." -f $i) 'Yellow'; Start-Sleep -Seconds 1 }

& (Join-Path $sys32 'rundll32.exe') user32.dll,LockWorkStation
Start-Sleep -Seconds 3
Say "锁屏已触发，运行 SYSTEM 探针任务..."
& (Join-Path $sys32 'schtasks.exe') /run /tn $taskName 2>&1 | Out-Null

$waited = 0
while ($waited -lt 45 -and -not (Test-Path $probeOut)) { Start-Sleep -Seconds 1; $waited++ }
Start-Sleep -Seconds 3
& (Join-Path $sys32 'schtasks.exe') /delete /tn $taskName /f 2>&1 | Out-Null

# --------------------------------------------------- 5. 证据判读（先看环境，再看现象）
Say ""
Say "=== 5. 证据判读 ==="
$sessionEvidence = 'unknown'; $desktopEvidence = 'unknown'; $sendEvidence = 'unknown'
if (Test-Path $probeOut) {
  $j = Get-Content $probeOut -Raw | ConvertFrom-Json
  $sessionEvidence = $j.session
  $desktopEvidence = $j.desktop
  $sendEvidence    = $j.sendProbe
  Say ("探针回报：session={0}  desktop={1}  isSystem={2}  sendProbe={3}" -f $j.session, $j.desktop, $j.isSystem, $j.sendProbe)
} else {
  Say "探针未产出结果文件（可能组件尚无 __inject-probe，或被安全桌面阻塞）" 'Yellow'
}
if (Test-Path $logOut) { Say ("任务输出：{0}" -f ((Get-Content $logOut -Raw).Trim() -split "`n" | Select-Object -First 3) -join ' | ') }

Say ""
Say "================================================================" 'Cyan'
Say " 请回答一个问题（这才是结论）：" 'White'
Say (" 锁屏密码框里**是否出现**了 {0} ？" -f $probe) 'White'
Say "================================================================" 'Cyan'
Say ""
if ($sessionEvidence -eq 0) {
  Say "⚠️ 注意：探针运行在 **session 0**（非交互桌面）。" 'Red'
  Say "   这种情况下"没出现探针"**不能**说明锁屏注入不可行 —— 这次测的是错路径。" 'Red'
  Say "   请回报这一行给我，我会改为实现真正的会话跳转（CreateProcessAsUser）后重测。" 'Red'
} elseif ($sessionEvidence -ne 'unknown' -and $sessionEvidence -ne $sessId) {
  Say ("⚠️ 探针会话号 {0} 与会话号 {1} 不一致，结论不可用，请回报我。" -f $sessionEvidence, $sessId) 'Yellow'
} else {
  Say "探针会话号与活动会话一致 → 本次测试走的是**真实交付路径**，结论可用。" 'Green'
}
Say ""
Say "如果现在还被锁在外面：在锁屏上手动输入密码即可。" 'Yellow'
Say ("临时文件：{0} / {1}（可删）" -f $probeOut, $logOut) 'Gray'
