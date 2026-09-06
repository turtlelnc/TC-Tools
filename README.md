# TC-tools

中文 | [English](https://github.com/turtlelnc/TC-Tools/blob/main/README_EN.md)

> 一个面向 **Windows 10 1709+** 的 C++ 控制台工具箱：工具链安装 · 工具链检查 · 内置轻量 CLI
>
> TC-Tools（全名 **Tclass-Tools**，TC 系列工具之一）\n>
> 当前版本：**v0.1.0-rc2** ｜ 语言：简体中文 / English（首页切换，默认中文）

TC-tools 是一个**不依赖任何第三方库、纯 Win32 API** 的 C++17/20 控制台应用（MinGW-w64 编译，静态链接，单文件 exe 即可运行）。
它把开发者常用工具链的**检测 / 安装 / 排查**集成到一个菜单里，自带了可对话的大模型轻量 CLI，并完全适配 Windows 10 1709（build 16299）。

---

## 功能一览

| 首页选项 | 说明 |
|---|---|
| 1. 工具链安装工具包 | 本地安装（用预先打包好的 packages 离线包，最快）或在线安装（自动下载最新版） |
| 2. 工具链检查及疑难解答 | 17 项检查：Node.js / Git / pnpm / MSYS2 / Deepseek Harness / Claude Code / Codex / Edge / Chrome / DEV-C++ / MinGW / Python / IDE / 办公软件 等 |
| 3. 内置轻量 CLI | 通过 API Key 连接大模型（DeepSeek / OpenAI / Kimi / GLM / 通义 / 硅基流动 / 火山方舟 / Ollama / 自定义），类 Claude Code 的流式对话，`/exit` 返回上一页 |
| 4. 切换语言 | 中文 ⇆ English（默认中文，选择会保存） |
| 5. 进入我们的网站 | 打开 https://turtleweb.cc.cd |
| 6. 退出 | 结束程序 |

所有安装项在安装前都会：
- **检查系统版本**（要求 Windows 10 1703 / build 15063 以上）；
- **检查是否已安装**，已安装则跳过，防止重复安装；
- 安装前/安装后打印对应版本号，并自动验证。

> 注：AI 套餐（codex + claude code）需要 Windows 10 1803+；在 Windows 10 1709 上会提示无法安装。
> JetBrains 2023.2 起官方要求 Windows 10 1809+；在 1709 上安装时会给出提示。

## Node.js 版（npm 安装）

本仓库同时提供了一个 **Node.js 版**（`node-app/`，功能与 C++ 版一致：工具链安装/检查/内置轻量 CLI、中英双语），
可从 npm 一条命令安装：

```bat
npm install -g @turtlelnc/tc-tools
tctool          :: 任意目录直接运行（命令名与 C++ 版一致）
```

- 包名：`@turtlelnc/tc-tools`（bin：`tctool`），仅依赖 Node 内置模块，无第三方依赖
- 详见 [node-app/README.md](node-app/README.md)

Node.js版介绍：[@turtlelnc/tc-tools](https://www.npmjs.com/package/@turtlelnc/tc-tools)

## 适用于 Windows 10 1709+ (x64) 的 C++控制台应用

请直接下载并使用 `TCtools-installer-0.1.0-rc2.exe` 安装包，具体内容请见 [Our Github Releases](https://github.com/turtlelnc/TC-Tools/releases)。

| 版本号 💾 | 系统 💻 | 点此跳转到对应的 Release 🔗 | 点此下载 ⬇️ |
| v0.1.0-rc2（最新） | Windows 10 1709+ x64 / Windows 11 x64 | [TC-tools v0.1.0-rc2](https://github.com/turtlelnc/TC-Tools/releases/tag/v0.1.0-rc2) | [TCtools-installer-0.1.0-rc2.exe](https://github.com/turtlelnc/TC-Tools/releases/download/v0.1.0-rc2/TCtools-installer-0.1.0-rc2.exe) |
| v0.1.0-rc1（旧版） | Windows 10 1709+ x64 / Windows 11 x64 | [TC-tools v0.1.0-rc1](https://github.com/turtlelnc/TC-Tools/releases/tag/v0.1.0-rc1) | [TCtools-installer-0.1.0-rc1.exe](https://github.com/turtlelnc/TC-Tools/releases/download/v0.1.0-rc1/TCtools-installer-0.1.0-rc1.exe) |

## 系统要求

- Windows 10 **1703（build 15063）或更高**；已针对 **Windows 10 1709（build 16299）** 做了完整适配（所有命令均为 1709 可用 API，无 Windows Terminal / 新版 PowerShell 依赖，curl 等系统组件不依赖 1803+ 内置版本）
- 建议 64 位系统（部分安装包仅提供 x64）

## 目录结构

```
TC-tools/
├── src/                    # 全部源代码（MinGW 编译，无第三方依赖）
│   ├── main.cpp            # 入口 + 主循环
│   ├── pages.cpp           # 菜单页面（首页/安装/检查/CLI）
│   ├── tools.cpp           # 工具链安装与检查逻辑
│   ├── cli.cpp             # 内置轻量 CLI（HTTP 流式对话）
│   ├── http.cpp            # WinHTTP 封装（下载/请求/流式，支持重定向与进度）
│   ├── json.cpp            # 迷你 JSON 解析/生成（UTF-8）
│   ├── util.cpp            # 控制台输出/进程/注册表/系统版本等 Win32 封装
│   └── lang.cpp            # 中英文语言表（约 140 条）
├── packages/               # 本地（离线）安装包清单 + 下载脚本
│   ├── manifest.json       # 本地安装包元数据（文件名/版本/来源 URL）
│   └── fetch-packages.ps1  # 一条命令把全部离线包下载到 packages/
├── installer/
│   ├── TCtools-installer.nsi   # NSIS 安装脚本（中文+英文双语）
│   └── build-installer.bat     # 一键生成安装程序
├── dist/                   # 构建产物（构建后生成，Git 忽略）
├── build.bat               # 一键编译（MinGW g++）
├── CMakeLists.txt          # 备用构建方式
└── README.md / README_EN.md / LICENSE
```

## 从源码构建

### 方式一：build.bat（推荐）

1. 安装 [MinGW-w64](https://winlibs.com/)（或使用 Qt 自带的 mingw1310_64 等）并确保 `g++` 在 PATH 中；
2. 双击或在 CMD 中执行：

```bat
build.bat
:: 或指定编译器路径
build.bat C:\Qt\Tools\mingw1310_64\bin\g++.exe
```

产物：`dist\tctool.exe`（静态链接，可直接拷贝运行，无 DLL 依赖）。

### 方式二：CMake

```bat
cmake -B build -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++
cmake --build build
```

### 方式三：直接 g++

```bat
g++ -std=c++20 -O2 -static -static-libgcc -static-libstdc++ ^
    src\main.cpp src\pages.cpp src\tools.cpp src\cli.cpp src\http.cpp ^
    src\json.cpp src\util.cpp src\app.cpp src\lang.cpp ^
    -o dist\tctool.exe -lwinhttp -lshell32 -luser32 -lversion -ladvapi32
```

## 使用说明

### 1. 工具链安装工具包 — 本地安装

- 最快：直接使用 `packages\` 目录下的离线安装包，不需要网络；
- 首次使用请先运行 `packages\fetch-packages.ps1` 下载全部离线包（或按 manifest 手动放置）；
- 每个选项都会显示版本号，安装前自动检查是否已安装。

### 2. 工具链安装工具包 — 在线安装

- 自动下载最新版（Git 走 GitHub Releases 最新标签、Node.js 走官方 latest-v22 目录、JetBrains 走官方下载通道等）；
- 显示下载进度；被运营商/镜像拦截时给出明确提示。

### 3. 工具链检查及疑难解答

- 第 1 项：一键总览已安装工具链；
- 第 2~15 项：单项检查（运行版本 / 依赖 / 路径，含失败原因建议）；
- 已安装项检测策略：优先 `where` + `--version`，其次注册表卸载项与常见安装路径。

### 4. 内置轻量 CLI

- 进入后选择厂商（DeepSeek / OpenAI / Kimi / 智谱 GLM / 阿里云百炼 / 硅基流动 / 火山方舟 / Ollama 本地 / 自定义）；
- **选厂商只需填 API Key**（Base URL 与模型已内置）；选“自定义”可填写所有参数；
- 类 Claude Code 的流式输出样式；`/exit` 返回上一页，`/clear` 清空对话，`/help` 帮助；
- 配置可保存到 `%APPDATA%\TC-tools\cli-config.json`（注意：Key 为明文，请自行管理）；
- 仅使用 WinHTTP（系统自带），无需 curl 等外部组件，Win10 1709 即可运行；
- 注意：内置 CLI 只调用**兼容 OpenAI 的 /chat/completions 接口**。

### 5. 安装程序 TCtools-installer

使用 NSIS 构建（中文 + English 双语）：

```bat
installer\build-installer.bat        :: 自动寻找 makensis（PATH 或参数）
makensis installer\TCtools-installer.nsi
```

安装行为：
- 安装全部运行文件（**不包含源代码**）到 `%ProgramFiles%\TC-tools`；
- **必选：添加到系统 PATH**，安装后可在 CMD 中直接运行 `tctool`；
- 可选：桌面快捷方式（默认不选）；
- 可选：开始菜单/应用列表条目（默认勾选）；
- 自带卸载程序（卸载时自动从 PATH 移除并广播环境变量）。

## 版本兼容说明（适配 Windows 10 1709）

| 组件 | 本工具使用的版本 | 备注 |
|---|---|---|
| Node.js | v22.23.2（LTS，>= Win10/Server 2016） | 在线安装走 latest-v22 通道 |
| MinGW-w64 | GCC 13.2.0 (WinLibs UCRT r8, POSIX-seh) | 解压到 %LOCALAPPDATA%\Programs\mingw64 并写入用户 PATH |
| Git for Windows | 2.55.0.5 | >= Win8.1，1709 完全可用 |
| MSYS2 | 2026-06-11 稳定版 | 官方要求 Win10 1607+ |
| pnpm | 10.x（npm -g） | 依赖 Node |
| Deepseek Harness | @deepseek-ai/dsh 0.1.x | 需要 Node 22.19+ 或 24+ |
| DEV-C++ | Bloodshed 5.11.0（含 TDM-GCC 4.9.2） | 1709 可用 |
| VS Code | 最新（用户安装版） | 支持 Win10 1607+ |
| Python | 3.13.15（含 IDLE） | 官方要求 Windows 10+ |
| PyCharm / CLion | 官方最新 | 官方要求 Win10 1809+（1709 上会提示） |
| JetBrains Toolbox | 官方最新 | 同上 |
| Qt | online installer（建议选 Qt 5.15.x） | Qt 6.x 官方要求 1809+ |
| Edge / Chrome | 最新 | Edge 官方支持 Win10 1709+ |
| WPS / QQ / 希沃白板 / 钉钉 / 微信 | 官方最新 | 在线安装打开官方页面 |

## 常见问题（FAQ）

- **中文乱码？** TC-tools 内部使用 UTF-8 + WriteConsoleW 直接输出，控制台代码页与字体不理想时也能正确显示中文；若终端字体缺中文字形，请更换字体（如 新宋体 / 微软雅黑）。
- **tctool 命令找不到？** 安装程序会把目录加入系统 PATH；如果是在安装前打开的 CMD，请重新打开终端。也可运行 `refreshenv`（无此命令则重开终端）。
- **本地安装提示缺少安装包？** 先运行 `packages\fetch-packages.ps1`。
- **在线下载被拦截？** 部分国内网络会拦截 SourceForge 系下载（如 DEV-C++），请用浏览器手动下载后放入 packages\。

## 免责声明

本工具**本体**（TC-tools）许可见 [LICENSE](LICENSE)。工具链安装包（Node.js、Git、MSYS2、JetBrains、Qt、Python、WPS、QQ、微信等）版权归各自原作者所有，本工具仅提供自动下载/运行入口，不附带任何默认支持。使用本工具安装第三方软件即表示您接受其各自的许可协议。

## 链接

- 官网：https://turtleweb.cc.cd
- License：Apache-2.0（详见 [LICENSE](LICENSE) 与 [NOTICE](NOTICE)）
