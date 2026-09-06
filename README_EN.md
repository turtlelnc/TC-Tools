# TC-tools

English | [中文](https://github.com/turtlelnc/TC-Tools/main/README.md)

> A C++ console toolkit for **Windows 10 1709+**: toolchain installer · toolchain checker · built-in lightweight AI CLI
>
> TC-Tools (full name **Tclass-Tools**, one of the TC series tools)\n>
> Current version: **v0.1.0-rc2** ｜ Language: Chinese / English (switchable on the home page; Chinese by default)

TC-tools is a console application built with pure Win32 API (**no third-party libraries**), compiled with MinGW-w64 and
statically linked — a single portable `tctool.exe`. It integrates **detection / installation / troubleshooting** of common
developer toolchains into one menu, ships a conversational lightweight LLM CLI, and is fully compatible with
**Windows 10 1709 (build 16299)**.

---

## Features

| Home page option | Description |
|---|---|
| 1. Toolchain install toolkit | Local install (pre-packaged offline files, fastest) or online install (auto-downloads the newest versions) |
| 2. Toolchain check & troubleshooting | 17 checks: Node.js / Git / pnpm / MSYS2 / Deepseek Harness / Claude Code / Codex / Edge / Chrome / DEV-C++ / MinGW / Python / IDEs / office software |
| 3. Built-in lightweight CLI | Connect to LLMs via API Key (DeepSeek / OpenAI / Kimi / GLM / Qwen / SiliconFlow / Volcengine Ark / Ollama / custom); Claude Code-like streaming chat; `/exit` goes back |
| 4. Switch language | Chinese ⇆ English (default Chinese; choice is persisted) |
| 5. Visit our website | Opens https://turtleweb.cc.cd |
| 6. Exit | Quit |

Every install action will:
- **Check the OS version** (requires Windows 10 1703 / build 15063 or later);
- **Check whether it is already installed** and skip it to avoid duplicates;
- Print the version before/after, then automatically verify.

> Note: the AI bundle (codex + claude code) requires Windows 10 1803+; on Windows 10 1709 it shows a "cannot install" notice.
> JetBrains 2023.2+ officially requires Windows 10 1809+; a notice is shown when installing on 1709.

## Node.js edition (npm)

This repository also ships a **Node.js edition** (`node-app/`, feature-equal to the C++ app:
toolchain install/check + built-in lightweight AI CLI, bilingual). Install it with one command:

```bat
npm install -g @turtlelnc/tc-tools
tctool          :: run anywhere (same command name as the C++ edition)
```

- Package: `@turtlelnc/tc-tools` (bin: `tctool`), zero third-party dependencies (Node built-ins only)
- Details: [node-app/README.md](node-app/README.md)
- 
Node.js version introduction: [@turtlelnc/tc-tools](https://www.npmjs.com/package/@turtlelnc/tc-tools)

## C++ console app for Windows 10 1709+ (x64)

Please directly download and use the `TCtools-installer-0.1.0-rc2.exe` installation package. For details, please see [Our Github Releases](https://github.com/turtlelnc/TC-Tools/releases).

| Version number 💾 | System 💻 | Click here to jump to the corresponding Release 🔗 | Click here to download ⬇️ |
|---|---|---|---|
| v0.1.0-rc2 (latest) | Windows 10 1709+ x64 / Windows 11 x64 | [TC-tools v0.1.0-rc2](https://github.com/turtlelnc/TC-Tools/releases/tag/v0.1.0-rc2) | [TCtools-installer-0.1.0-rc2.exe](https://github.com/turtlelnc/TC-Tools/releases/download/v0.1.0-rc2/TCtools-installer-0.1.0-rc2.exe) |
| v0.1.0-rc1 (old version) | Windows 10 1709+ x64 / Windows 11 x64 | [TC-tools v0.1.0-rc1](https://github.com/turtlelnc/TC-Tools/releases/tag/v0.1.0-rc1) | [TCtools-installer-0.1.0-rc1.exe](https://github.com/turtlelnc/TC-Tools/releases/download/v0.1.0-rc1/TCtools-installer-0.1.0-rc1.exe) |

## System requirements

- Windows 10 **1703 (build 15063) or later**; fully tested for **Windows 10 1709 (build 16299)** —
  every command only uses APIs available in 1709 (no Windows Terminal / modern PowerShell / built-in curl dependency).
- A 64-bit system is recommended (some installers are x64 only).

## Repository layout

```
TC-tools/
├── src/                    # all source code (MinGW, no third-party deps)
│   ├── main.cpp            # entry + main loop
│   ├── pages.cpp           # menu pages (home / install / check / CLI)
│   ├── tools.cpp           # toolchain install & check logic
│   ├── cli.cpp             # built-in lightweight CLI (HTTP streaming chat)
│   ├── http.cpp            # WinHTTP wrapper (download / request / streaming)
│   ├── json.cpp            # minimal JSON parser/serializer (UTF-8)
│   ├── util.cpp            # console / process / registry / OS-version Win32 helpers
│   └── lang.cpp            # bilingual language table (about 140 entries)
├── packages/               # local (offline) install package manifest + fetch script
├── installer/              # NSIS installer (TCtools-installer)
├── build.bat               # one-click build (MinGW g++)
├── CMakeLists.txt          # alternative build
└── README.md / README_EN.md / LICENSE
```

## Building from source

### Option 1: build.bat (recommended)

1. Install [MinGW-w64](https://winlibs.com/) and make sure `g++` is on PATH;
2. Run:

```bat
build.bat
:: or point to a specific compiler
build.bat C:\Qt\Tools\mingw1310_64\bin\g++.exe
```

Output: `dist\tctool.exe` (statically linked, single file, no DLL dependencies).

### Option 2: CMake

```bat
cmake -B build -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER=g++
cmake --build build
```

### Option 3: plain g++

```bat
g++ -std=c++20 -O2 -static -static-libgcc -static-libstdc++ ^
    src\main.cpp src\pages.cpp src\tools.cpp src\cli.cpp src\http.cpp ^
    src\json.cpp src\util.cpp src\app.cpp src\lang.cpp ^
    -o dist\tctool.exe -lwinhttp -lshell32 -luser32 -lversion -ladvapi32
```

## Usage

### 1. Toolchain toolkit — Local install

- Uses the pre-packaged installers under `packages\`; no network needed;
- Run `packages\fetch-packages.ps1` once to download all offline packages;
- Every option shows its version and checks for an existing install before running.

### 2. Toolchain toolkit — Online install

- Fetches the newest builds (Git via GitHub Releases, Node.js via the official latest-v22 directory,
  JetBrains via the official download API, ...);
- Shows download progress and a clear message if a CDN/mirror blocks the download.

### 3. Toolchain check & troubleshooting

- Item 1: one-glance summary of installed toolchains;
- Items 2-15: individual checks (version / dependencies / path, with hints on failure);
- Detection: `where` + `--version` first, then registry uninstall entries and common install paths.

### 4. Built-in lightweight CLI

- Pick a vendor (DeepSeek / OpenAI / Kimi / Zhipu GLM / Alibaba DashScope / SiliconFlow / Volcengine Ark / Ollama / custom);
- **Vendors only require your API Key** (Base URL and model are preset); "custom" lets you fill in everything;
- Claude Code-like streaming output; `/exit` returns to the previous page, `/clear` clears the chat, `/help` shows help;
- Config can be saved to `%APPDATA%\TC-tools\cli-config.json` (note: the Key is stored in plain text);
- Uses only WinHTTP (built into Windows) — no external curl tool required, runs on Win10 1709.

### 5. TCtools-installer (NSIS)

Build with NSIS (Chinese + English):

```bat
installer\build-installer.bat        :: auto-detects makensis (PATH or argument)
makensis installer\TCtools-installer.nsi
```

Installer behavior:
- Installs all runtime files (**no source code**) to `%ProgramFiles%\TC-tools`;
- **Mandatory: adds the folder to the system PATH** — run `tctool` from any CMD afterwards;
- Optional: desktop shortcut (unchecked by default);
- Optional: Start Menu / app list entry (checked by default);
- Bundles an uninstaller that also removes the PATH entry and broadcasts the environment change.

## Version & compatibility notes (Windows 10 1709)

| Component | Version used by this tool | Notes |
|---|---|---|
| Node.js | v22.23.2 (LTS, >= Win10/Server 2016) | online install uses the latest-v22 channel |
| MinGW-w64 | GCC 13.2.0 (WinLibs UCRT r8, POSIX-seh) | extracted to %LOCALAPPDATA%\Programs\mingw64, user PATH |
| Git for Windows | 2.55.0.5 | supports 1709 fully |
| MSYS2 | 2026-06-11 stable | requires Win10 1607+ |
| pnpm | 10.x (npm -g) | needs Node |
| Deepseek Harness | @deepseek-ai/dsh 0.1.x | needs Node 22.19+ or 24+ |
| DEV-C++ | Bloodshed 5.11.0 (with TDM-GCC 4.9.2) | works on 1709 |
| VS Code | latest (user setup) | Win10 1607+ |
| Python | 3.13.15 (with IDLE) | Win10+ |
| PyCharm / CLion | official latest | official requires Win10 1809+ (notice on 1709) |
| JetBrains Toolbox | official latest | see above |
| Qt | online installer (choose Qt 5.15.x for 1709) | Qt 6.x requires 1809+ |
| Edge / Chrome | latest | Edge officially supports Win10 1709+ |
| WPS / QQ / Seewo / DingTalk / WeChat | official latest | online install opens the official page |

## FAQ

- **Garbled Chinese?** TC-tools outputs via UTF-8 + WriteConsoleW — correct even with an unfriendly console code page.
  If glyphs are missing, change the console font (e.g. NSimSun / Microsoft YaHei).
- **`tctool` not found?** The installer adds the directory to the system PATH; reopen your terminal (or use `refreshenv`).
- **Local install says package missing?** Run `packages\fetch-packages.ps1` first.
- **Download blocked?** Some networks block SourceForge-based downloads (e.g. DEV-C++); download manually with a browser
  and drop the file into `packages\`.

## Disclaimer

The TC-tools **application itself** is licensed under [LICENSE](LICENSE). The toolchain installers (Node.js, Git, MSYS2,
JetBrains, Qt, Python, WPS, QQ, WeChat, ...) belong to their respective owners; TC-tools only provides a convenience
auto-download/launch entry and does not bundle or endorse them. Installing third-party software through this tool means
you accept their respective license agreements.

## Links

- Website: https://turtleweb.cc.cd
- License: Apache-2.0 (see [LICENSE](LICENSE) and [NOTICE](NOTICE))
