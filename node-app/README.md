# tc-tools (Node.js 版 / Node.js edition)

TC-tools（全名 **Tclass-Tools**，TC 系列工具之一）的 **Node.js 重制版**：与原版 C++ 控制台应用功能一致
（Windows 10 1709+ 工具链安装/检查/内置轻量 CLI、中英双语），可通过 npm 一条命令安装：

```bat
npm install -g @turtlelnc/tc-tools
rem 安装后任意目录直接运行：
tctool
```

## 功能

- 首页：① 工具链安装工具包（本地/在线） ② 工具链检查及疑难解答（17 项） ③ 内置轻量 CLI（流式对话）
  ④ 切换语言（中文/English，默认中文） ⑤ 打开官网 https://turtleweb.cc.cd ⑥ 退出
- 所有安装预检查系统版本（Win10 1703+）与是否已安装，防止重复安装
- AI 套餐（codex + claude code）在 Windows 10 1709 及更早时提示无法安装（需 1803+）
- 内置 CLI：DeepSeek / OpenAI / Kimi / 智谱 GLM / 阿里云百炼 / 硅基流动 / 火山方舟 / Ollama（本地）/ 自定义；
  选厂商只需填 API Key；`/exit` 返回上一页；配置保存于 %APPDATA%\TC-tools\cli-config.json
  （与原 C++ 版格式兼容，可共用配置）

## 与 C++ 版的关系

- 功能等效，交互与文案保持一致；本包**不依赖任何第三方 npm 包**（只用 Node 内置模块）
- 本包默认不使用 ANSI 颜色（保证 Win10 1709 传统控制台显示正常）；设置环境变量 `TCTOOL_COLOR=1` 可开启彩色输出
- 本地安装包：默认查找 `%LOCALAPPDATA%\TC-tools\packages`（或用环境变量 `TCTOOL_PACKAGES` 指定目录），
  可先运行仓库中的 `packages\fetch-packages.ps1` 下载离线包，或对每个选项使用【在线安装】

## 开发

```bat
git clone https://github.com/turtlelnc/TC-Tools.git
cd TC-tools\node-app
npm install        （无依赖，仅为开发用）
node bin\tctool.js
```

发布到 npm（需要 npm 账号登录）：

```bat
npm login
npm publish
```

License: Apache-2.0