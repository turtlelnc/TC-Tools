# packages/ - 本地（离线）安装包

本目录存放 TC-tools「本地安装」所需的预打包安装包（由 manifest.json 描述）。
TC-tools 仓库不包含这些二进制文件（体积大，请勿提交到 Git）。

## 使用

```powershell
cd packages
powershell -ExecutionPolicy Bypass -File .\fetch-packages.ps1
```

脚本会按 manifest.json 下载全部离线包到本目录（Node.js、MinGW-w64 13.2、Git、MSYS2、
DEV-C++、VS Code、Python、JetBrains、Qt 等）。被网络拦截的包（如 SourceForge 系）请用
浏览器手动下载后放入本目录，文件名与 manifest.json 中一致即可。

