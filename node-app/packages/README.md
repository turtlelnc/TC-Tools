# node-app/packages - 本地安装包清单（参考）

与仓库根目录 packages/ 的内容一致。npm 版 tc-tools 的【本地安装】默认查找：
- 环境变量 `TCTOOL_PACKAGES` 指定的目录
- %LOCALAPPDATA%\TC-tools\packages （默认）

下载离线包：`powershell -ExecutionPolicy Bypass -File packages\fetch-packages.ps1`（在仓库根目录）。
