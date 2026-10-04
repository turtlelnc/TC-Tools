# TC-tools v0.2.0-rc2 发布说明 | TC-tools v0.2.0-rc2 Release Notes

**语言 / Language**：[中文部分](#中文部分--chinese-part) ｜ [English Part](#english-part--英文部分)

---

## 中文部分 | Chinese Part

> ⚠️ **重要：本版本（v0.2.0-rc2）的键盘注入存在功能性缺陷，请勿据此判断"解锁能否使用"。**
>
> **现象**：解锁密码以**一次性爆发（burst）**方式注入，可能导致**密码被截断**（实测：13 个字符仅送达 6 个）。
> **后果**：解锁会失败；而且失败表现**酷似"Windows 拒绝安全桌面注入"**，极易被误判为"手机解锁电脑不可行"。
> **原因**：`tcyunlock/src/Win/InputInjector.cs` 的注释声称"键间有 10ms 间隔以适配登录界面"，
> 但代码中的 `Thread.Sleep` 位于**拼装事件数组**的循环里，**从未落在两次注入之间**；真实注入是单次 `SendInput` 批量投递。
> （这是**注释描述了意图、代码没有实现**的典型情形。）
>
> **修复状态**：已在 **rc3 开发中**修复 —— 改为**逐键一次 `SendInput` + 真实间隔**，实测逐字符一致
> （修复前 13→6，修复后 `eventsSent=16/16`、`probe123` 8/8）。
>
> **因此**：
> - 本版本的**协议/加密层结论有效**（5 路独立实现交叉一致、两端互通全过）；
> - 但 **"能否解锁电脑/锁屏"必须以 rc3 或更高版本实测为准**；
> - 冻结产物 `tcyunlock/artifacts/tctool-unlock-0.2.0-rc2-2BC9BB1C.exe` 已标注
>   `KNOWN FUNCTIONAL DEFECT IN THIS BINARY (rc2; fixed in rc3)` —— **不要把它当作可用版本发布**。
>
> 详见 `tests/unlock/verify-report.md` §13 与 `tcyunlock/README.md` §6。

> 本版本新增 **手机指纹蓝牙解锁电脑**：手机端 App「TC-Tools 解锁电脑」验证指纹后，通过蓝牙让电脑自动输入密码解锁。
> 同时提供 Windows 端用户可设置的自启动。

---

## 一、新增功能

### 1. 手机指纹蓝牙解锁（两端）

| 端 | 内容 |
|---|---|
| **手机端（Android 8.0+）** | 应用名「TC-Tools 解锁电脑」，Kotlin + Jetpack Compose，**Apple 风格 UI**；指纹验证（BiometricPrompt，支持设备凭据回退）；BLE Central；扫码/手动粘贴配对；**控制中心快捷磁贴**一键解锁（含 realme/OPPO/小米等厂商后台适配） |
| **电脑端（并入 TC-tools）** | 首页新增 **4. 解锁电脑（蓝牙）**；实际执行由 `unlock\tctool-unlock.exe`（.NET 8，自包含单文件）完成：BLE GATT Server、加密握手、DPAPI 密码保险箱、SendInput 注入、二维码配对、离线自测 |

#### 使用流程

1. 电脑：`4 解锁电脑（蓝牙）` → `2 设置电脑解锁密码`（当前登录密码，DPAPI 加密保存）；
2. 电脑：`1 配对手机` → 生成二维码（含一次性密钥）；
3. 手机：扫码配对（或粘贴配对文本）；
4. 手机：点主按钮或控制中心磁贴 → **指纹验证通过** → 蓝牙发加密指令 → 电脑自动输密码解锁。

### 2. 电脑端自启动（用户可设置，默认关闭）

```bat
tctool-unlock autostart status     :: 查看（真实探测系统状态）
tctool-unlock autostart enable     :: 开启（用户级，无需管理员）
tctool-unlock autostart disable    :: 关闭（三种机制一次清理）
```

- 默认**关闭**，不会偷偷自启；用户可在 TC-tools 里开关；
- 用户级实现：优先计划任务 → 回退启动文件夹快捷方式 → HKCU Run（`--method` 可强制）；
- `run --quiet` 无控制台窗口，日志写 `%LOCALAPPDATA%\TC-tools\unlock\service.log`（1 MB 滚动）。

---

## 二、安全设计（要点）

- **密码不出电脑**：PC 密码用 Windows DPAPI 加密保存在本机，**任何情况下都不通过蓝牙或网络传输**；
- **配对密钥只走二维码**：PSK（32 字节随机）经二维码/配对文本传递，不经蓝牙明文传输；
- **一次性会话**：每次连接重新生成 32 字节随机挑战值；会话密钥 `HMAC-SHA256(PSK, NONCE‖"TCUNLOCK-SESSION-V1")`；
- **挑战-应答认证**：`PROOF = HMAC-SHA256(PSK, NONCE‖"TCUNLOCK-PROOF-V1"‖HOST_ID‖PEER_ID)`，常量时间比较；
- **解锁指令全部加密**：AES-256-GCM（IV = 计数器大端 8 字节 + 4 字节 `0x00`），**单调计数器防重放**；
- **防暴力破解**：连续 5 次验证失败 → **立即作废配对密钥**，需重新配对；
- **指纹前置**：PROOF 只在指纹验证成功的回调里计算并发送（代码层已核对唯一调用点）。

> **局限（如实说明）**：蓝牙采用**应用层加密**，未启用系统级 BLE 配对，因此不防“信号中继”式攻击；
> 请勿在人员复杂的公共场所长期开启服务。

### 关于锁屏注入

Windows **安全桌面**不接受普通用户进程的模拟输入。

- 若密码框出现在安全桌面，需要把 `tctool-unlock.exe` 以 **SYSTEM** 身份常驻（见 `tcyunlock/README.md`）；
- 普通“屏幕保护式锁定”可正常注入。

---

## 三、版本与兼容性

- 版本号：**v0.2.0-rc2**（C++ 端 / Node.js 版 / 手机端 / 电脑端解锁组件统一）；
- 协议版本：`protocol = 1`（与 v0.1.0-rc* 的解锁协议一致，未破坏兼容）；
- 系统要求：Windows 10 **1709（build 16299）** 及以上；手机端 Android 8.0+；
- 电脑端解锁组件为 **self-contained**，目标机**无需预装 .NET 8 运行时**。

---

## 四、产物清单

| 产物 | 说明 |
|---|---|
| `dist\TCtools-installer-0.2.0-rc2.exe` | Windows 安装程序（NSIS，中英双语；含解锁组件到 `unlock\` 子目录） |
| `dist\tctool.exe` | C++ 控制台主程序（静态单文件，可独立使用） |
| `dist\android\TC-Tools-Unlock-0.2.0-rc2.apk` | 手机端安装包 |
| `tcyunlock\dist\tctool-unlock.exe` | 电脑端蓝牙解锁组件（自包含） |
| npm `@turtlelnc/tc-tools@0.2.0-rc2` | Node.js 版（`npm install -g @turtlelnc/tc-tools`） |

> 大文件（安装程序 / APK / exe）不进 Git，通过 **GitHub Release** 分发。

---

## 五、验证情况（诚实口径）

### 已验证（有可复现证据）

- 协议正确性：**5 条独立实现**（node:crypto、手工 HMAC+AES+GHASH、CPython、OpenSSL CLI、.NET CNG）算出的
  `K_session / PROOF / SEAL 帧` **逐字节一致**；两端**双向互通**（互相解密对方的帧）通过；
- 两端真实构建：C++ 用 MinGW g++ 编译通过并实跑；电脑端组件 `selftest` **29/29 通过**；Android
  `assembleDebug` 成功、单元测试 8/8、自测 24/24；
- Android APK 在**真实 Android 14 运行时（模拟器）**安装成功、App 正常启动、**协议自测 24/24 通过**；
- 电脑端 BLE **GATT 服务真实广播成功**（`AdvertisementStatusChanged -> Started`）；
- 自启动三次状态实测（默认关 → 开启 → 关闭，且用独立工具读回快捷方式验证）；
- 二维码经 **OpenCV 独立解码 5/5** 逐字节还原（含中文电脑名）。

### 未验证（本机条件所限，如实列出）

1. **真机蓝牙空口链路**：单适配器无法自连，模拟器蓝牙非真实射频 —— 需要一台真手机 + 一台电脑实测；
2. **真实生物识别硬件**：模拟器 `adb emu finger touch` 不等于真机指纹传感器路径；
3. **真实锁屏 / 安全桌面的注入**：需要真的锁屏并注入，会打断当前会话且涉及真实密码，未在本机执行；
4. **Windows 10 1709 运行时**：本机为 Windows 11 26200，仅确认 API 与目标框架兼容（未用 19041+ 独有 API）；
5. **真实重启后自启动是否按时拉起**：计划任务与 HKCU Run 两条路径在本机因权限/策略被拒，未能稳定复现。

> **建议的真机验收清单**：见 [`docs/UNLOCK-VERIFY.md`](UNLOCK-VERIFY.md)。

---

## 六、已知问题与说明

- 未做代码签名，Windows SmartScreen 可能提示“未知发布者”（计划申请开源代码签名证书）；
- 电脑端首次运行需保持蓝牙**已打开**（组件会检测电台状态并在关闭时给出明确提示）；
- 手机端在没有系统“后台弹出界面”权限的机型上，磁贴点击会先拉起 App 再弹指纹
  （realme/OPPO/小米等，见 `android/README.md`）；
- **`forget` 不会清理已导出的二维码文件**（rc2 行为）：`forget` 之后残留的 `payload.txt` / `payload.png`
  里是**已作废的 PSK**，用户若再扫旧码只会认证失败一次
  （**无安全影响**：`host.json` 中已无可用密钥材料，无 PSK 时认证必然失败）。
  **rc3 起 `forget` 会自动删除这两个文件**（源码已就绪，随下次构建生效）；
- 自启动的三种机制中，**本机实际生效的是“启动文件夹快捷方式”**（计划任务被 `Access is denied.` 拒绝、
  `HKCU\...\Run` 在本机会话被安全策略间歇性拦截，二者均已实现但本机无法稳定复现成功）。
  关闭方式：
  - 任务栏右键 → 任务管理器 → **启动应用** → `TC-tools Unlock Service` → 禁用；
  - 或 `Win+R` → `shell:startup` → 删除 `TC-tools Unlock Service.lnk`。

  详见 `tcyunlock/README.md` §13。

---

## 七、产物 ↔ 源码对应关系

见 [`docs/BUILD-MANIFEST-v0.2.0-rc2.md`](BUILD-MANIFEST-v0.2.0-rc2.md)：

- **commit 标识“源码身份”（审计用）**，**SHA-256 标识“被交付的那个文件”**（验证以它为准）；
- **不要用“从同一提交重建”来证明与已验证产物相同** —— 实测发现本项目的自包含单文件产物**对构建环境敏感**：
  同一台机、同一 SDK、同源码，仅改变**输出目录名长度**就会让产物相差几个字节；
  且本仓库 `core.autocrlf=true` 会使 `git checkout` 落成 CRLF，而冻结产物由 **LF** 源码构建。
  **重新构建应视为新产物，须重新记录哈希并重新验证功能。**
- 另显式记录：`tcyunlock/src/Program.cs` 在 HEAD 上含一项 rc3 改动
  （`forget` 清理过期导出物），但**二进制故意未重建**（rc2 冻结哈希因此有效）。

---

## English Part | 英文部分

> This release adds **phone-fingerprint Bluetooth PC unlock**: after the “TC-Tools Unlock PC” phone app verifies
> your fingerprint, it makes the PC type your password over Bluetooth and unlock.
> A user-configurable autostart for Windows is included as well.

---

## 1. New features

### 1.1 Phone-fingerprint Bluetooth unlock (both ends)

| End | Details |
|---|---|
| **Phone (Android 8.0+)** | App name “TC-Tools Unlock PC” (「TC-Tools 解锁电脑」), Kotlin + Jetpack Compose, **Apple-style UI**; fingerprint verification (BiometricPrompt, with device-credential fallback); BLE Central; pairing by QR scan or manual paste; **Quick Settings tile** for one-tap unlock (including background restrictions handled on realme/OPPO/Xiaomi and similar OEMs) |
| **PC (integrated into TC-tools)** | New home-page entry **4. Unlock PC (Bluetooth)**; the actual work is done by `unlock\tctool-unlock.exe` (.NET 8, self-contained single file): BLE GATT server, encrypted handshake, DPAPI password vault, SendInput injection, QR-code pairing, offline self-test |

#### How to use

1. PC: `4 Unlock PC (Bluetooth)` → `2 Set PC unlock password` (your current sign-in password, stored encrypted with DPAPI);
2. PC: `1 Pair phone` → generates a QR code (containing a one-time key);
3. Phone: scan the QR code to pair (or paste the pairing text);
4. Phone: tap the main button or the Quick Settings tile → **fingerprint verified** → an encrypted command is sent over Bluetooth → the PC types the password and unlocks.

### 1.2 PC autostart (user-configurable, off by default)

```bat
tctool-unlock autostart status     :: query (really probes the system state)
tctool-unlock autostart enable     :: enable (user level, no administrator needed)
tctool-unlock autostart disable    :: disable (cleans up all three mechanisms at once)
```

- **Off** by default — it never starts behind your back; you can toggle it inside TC-tools;
- User-level implementation: scheduled task first → falls back to a Startup-folder shortcut → HKCU Run
  (`--method` forces one of them);
- `run --quiet` shows no console window and logs to `%LOCALAPPDATA%\TC-tools\unlock\service.log` (1 MB rolling).

---

## 2. Security design (highlights)

- **The password never leaves the PC**: the PC password is stored on that machine encrypted with Windows DPAPI and is
  **never transmitted over Bluetooth or the network under any circumstances**;
- **The pairing key travels only in the QR code**: the PSK (32 random bytes) is delivered through the QR code /
  pairing text, never in plaintext over Bluetooth;
- **One-time sessions**: a fresh 32-byte random challenge is generated for every connection; the session key is
  `HMAC-SHA256(PSK, NONCE‖"TCUNLOCK-SESSION-V1")`;
- **Challenge–response authentication**: `PROOF = HMAC-SHA256(PSK, NONCE‖"TCUNLOCK-PROOF-V1"‖HOST_ID‖PEER_ID)`,
  compared in constant time;
- **Every unlock command is encrypted**: AES-256-GCM (IV = big-endian 8-byte counter + 4 bytes of `0x00`), with a
  **monotonic counter for replay protection**;
- **Brute-force protection**: 5 consecutive failed verifications → the **pairing key is invalidated immediately** and
  you have to pair again;
- **Fingerprint comes first**: PROOF is computed and sent only inside the callback that runs after fingerprint
  verification succeeds (the single call site was audited in the code).

> **Limitation (stated honestly)**: Bluetooth uses **application-layer encryption** and does not enable OS-level BLE
> pairing, so relay (“signal relay”) attacks are not mitigated; do not leave the service running for long periods in
> crowded public places.

### About injection on the lock screen

Windows' **secure desktop** does not accept simulated input from a normal user process.

- If the password box appears on the secure desktop, `tctool-unlock.exe` must run resident as **SYSTEM**
  (see `tcyunlock/README.md`);
- An ordinary “screen-saver style” lock accepts injection normally.

---

## 3. Version & compatibility

- Version: **v0.2.0-rc2** (unified across the C++ app / Node.js edition / phone app / PC unlock component);
- Protocol version: `protocol = 1` (same unlock protocol as v0.1.0-rc*, no compatibility break);
- Requirements: Windows 10 **1709 (build 16299)** or later; Android 8.0+ on the phone;
- The PC unlock component is **self-contained**: target machines do **not** need the .NET 8 runtime preinstalled.

---

## 4. Artifacts

| Artifact | Description |
|---|---|
| `dist\TCtools-installer-0.2.0-rc2.exe` | Windows installer (NSIS, Chinese + English; ships the unlock component into the `unlock\` subdirectory) |
| `dist\tctool.exe` | C++ console main program (statically linked single file, usable standalone) |
| `dist\android\TC-Tools-Unlock-0.2.0-rc2.apk` | Phone app package |
| `tcyunlock\dist\tctool-unlock.exe` | PC-side Bluetooth unlock component (self-contained) |
| npm `@turtlelnc/tc-tools@0.2.0-rc2` | Node.js edition (`npm install -g @turtlelnc/tc-tools`) |

> Large files (installer / APK / exe) are not committed to Git; they are distributed through **GitHub Releases**.

---

## 5. Verification status (honest account)

### Verified (with reproducible evidence)

- Protocol correctness: **5 independent implementations** (node:crypto, hand-rolled HMAC+AES+GHASH, CPython,
  the OpenSSL CLI, .NET CNG) produced **byte-identical** `K_session / PROOF / SEAL frames`; **two-way interop**
  between the two ends (each decrypting the other's frames) passes;
- Real builds on both ends: the C++ app compiles and actually runs under MinGW g++; the PC component's `selftest`
  passes **29/29**; Android `assembleDebug` succeeds, unit tests 8/8, self-test 24/24;
- The APK installs on a **real Android 14 runtime (emulator)**, the app launches normally and the
  **protocol self-test passes 24/24**;
- The PC-side BLE **GATT service really does advertise successfully** (`AdvertisementStatusChanged -> Started`);
- Autostart was measured in all three states (off by default → enable → disable, with the shortcut read back by an
  independent tool);
- The QR code was independently decoded by **OpenCV, 5/5**, restoring the payload byte-for-byte
  (including a Chinese PC name).

### Not verified (limitations of this machine, listed honestly)

1. **Real over-the-air Bluetooth link**: a single adapter cannot connect to itself, and emulator Bluetooth is not a
   real radio — this needs one real phone plus one real PC;
2. **Real biometric hardware**: `adb emu finger touch` on an emulator is not the same code path as a real fingerprint
   sensor;
3. **Injection on a real lock screen / secure desktop**: doing it for real would interrupt the current session and
   involves the real password, so it was not performed here;
4. **Running on Windows 10 1709**: this machine is Windows 11 26200; only API and target-framework compatibility was
   confirmed (no 19041+ only APIs are used);
5. **Whether autostart really comes up on time after a reboot**: the scheduled-task and HKCU Run paths were refused on
   this machine by permissions/policy, so they could not be reproduced reliably.

> **Recommended on-device acceptance checklist**: see [`docs/UNLOCK-VERIFY.md`](UNLOCK-VERIFY.md).

---

## 6. Known issues & notes

- The binaries are not code-signed yet, so Windows SmartScreen may warn about an “unknown publisher”
  (an open-source code-signing certificate is planned);
- On first run the PC component requires Bluetooth to be **switched on** (it checks the radio state and gives a clear
  message when the radio is off);
- On phones without the system “pop up in background” permission, tapping the tile brings the app to the foreground
  before the fingerprint prompt appears (realme/OPPO/Xiaomi and similar; see `android/README.md`);
- **`forget` does not clean up QR payloads that were already exported** (rc2 behaviour): after `forget`, the leftover
  `payload.txt` / `payload.png` hold a **revoked PSK**, and scanning that old code again only causes a single
  authentication failure (**no security impact**: `host.json` no longer contains any usable key material, and
  authentication must fail when there is no PSK).
  **From rc3 on, `forget` deletes both files automatically** (the source change is ready and takes effect with the
  next build);
- Of the three autostart mechanisms, the one that actually takes effect on this machine is the
  **Startup-folder shortcut** (the scheduled task was refused with `Access is denied.`, and `HKCU\...\Run` is
  intermittently blocked by security policy in this session; both are implemented but could not be reproduced
  reliably here). To turn it off:
  - right-click the taskbar → Task Manager → **Startup apps** → `TC-tools Unlock Service` → Disable;
  - or `Win+R` → `shell:startup` → delete `TC-tools Unlock Service.lnk`.

  See `tcyunlock/README.md` §13.

---

## 7. Artifact ↔ source mapping

See [`docs/BUILD-MANIFEST-v0.2.0-rc2.md`](BUILD-MANIFEST-v0.2.0-rc2.md):

- **The commit identifies “which source” (for auditing)**; **the SHA-256 identifies “the file that was actually
  delivered”** (verification is based on the hash);
- **Do not use “rebuild from the same commit” to prove a build equals the verified artifact** — in practice this
  project's self-contained single-file artifacts are **sensitive to the build environment**: on the same machine,
  with the same SDK and the same source, merely changing the **length of the output directory name** makes the
  artifact differ by a few bytes; and this repository's `core.autocrlf=true` makes `git checkout` write CRLF sources,
  while the frozen artifacts were built from **LF** sources.
  **Treat a rebuild as a new artifact: record its hash again and re-verify its behaviour.**
- Also recorded explicitly: on HEAD, `tcyunlock/src/Program.cs` contains one rc3 change (`forget` clearing expired
  exported payloads), but the **binary was deliberately not rebuilt** (which is why the frozen rc2 hashes remain
  valid).
