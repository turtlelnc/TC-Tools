// tools.js - toolchain checks & installs (TC-tools Node.js edition)
'use strict';
const path = require('path');
const fs = require('fs');
const core = require('./core');
const { print, println, fmt, runCmd, whereCmd, runInstall, runDetach, openUrl,
  appDir, packagesDir, fileExists, writeText, osGuard, findUninstall,
  userPathAdd, fileVersionStr, download, firstLine, is1709OrOlder, buildAtLeast, winVerName } = core;

// ---------------------------------------------------------------- checks ----
function chkFromCmd(whereName, verArgs) {
  const w = whereCmd(whereName);
  if (!w) return { found: false };
  const isScript = /\.(cmd|bat|ps1)$/i.test(w);
  const r = runCmd(isScript ? ('cmd /c "' + w + '" ' + verArgs) : ('"' + w + '" ' + verArgs), 25000);
  return { found: true, where: w, ver: (r.ran && r.code === 0 && r.out.trim() ? firstLine(r.out) : '?') };
}
function chkNode() { return chkFromCmd('node', '--version'); }
function chkGit() { return chkFromCmd('git', '--version'); }
function chkPnpm() { return chkFromCmd('pnpm', '--version'); }
function chkDsh() {
  const w = whereCmd('dsh');
  if (!w) return { found: false };
  const r = runCmd('cmd /c dsh --version', 60000);
  return { found: true, where: w, ver: (r.ran && r.code === 0 ? firstLine(r.out) : '') };
}
function chkClaude() { return chkFromCmd('claude', '--version'); }
function chkCodex() { return chkFromCmd('codex', '--version'); }
function chkMsys2() {
  const bash = 'C:\\msys64\\usr\\bin\\bash.exe';
  if (fileExists(bash)) {
    const r = runCmd('"' + bash + '" --version', 15000);
    return { found: true, where: bash, ver: r.ran ? firstLine(r.out) : '?' };
  }
  const u = findUninstall('MSYS2');
  if (u.found) return { found: true, where: u.name, ver: u.version };
  const w = whereCmd('bash');
  return w ? { found: true, where: w, ver: 'found in PATH' } : { found: false };
}
function chkAppPath(name, paths, whereName) {
  for (const p of paths) {
    try { if (fs.existsSync(p)) return { found: true, where: p, ver: fileVersionStr(p) || '?' }; } catch (e) { /* ignore */ }
  }
  if (whereName) { const w = whereCmd(whereName); if (w) return { found: true, where: w, ver: '?' }; }
  const u = findUninstall(name);
  if (u.found) return { found: true, where: u.name, ver: u.version };
  return { found: false };
}
function chkEdge() {
  return chkAppPath('Microsoft Edge', [
    'C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe',
    'C:\\Program Files\\Microsoft\\Edge\\Application\\msedge.exe',
    'C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\MicrosoftEdge.exe'], null);
}
function chkChrome() {
  const la = process.env.LOCALAPPDATA || '';
  return chkAppPath('Google Chrome', [
    'C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe',
    'C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe',
    la + '\\Google\\Chrome\\Application\\chrome.exe'], null);
}
function chkDevCpp() {
  const c = chkAppPath('Dev-C++', ['C:\\Program Files (x86)\\Dev-Cpp\\devcpp.exe', 'C:\\Dev-Cpp\\devcpp.exe'], null);
  if (!c.found) { const u = findUninstall('Dev-C++'); if (u.found) return { found: true, where: u.name, ver: u.version }; }
  return c;
}
function chkMingw() {
  const la = process.env.LOCALAPPDATA || '';
  const cands = ['C:\\mingw64\\bin\\gcc.exe', la + '\\Programs\\mingw64\\bin\\gcc.exe',
    'C:\\msys64\\mingw64\\bin\\gcc.exe', 'C:\\msys64\\usr\\bin\\gcc.exe'];
  for (const p of cands) {
    if (fileExists(p)) {
      const r = runCmd('"' + p + '" --version', 15000);
      return { found: true, where: p, ver: r.ran && r.code === 0 ? firstLine(r.out) : '?' };
    }
  }
  const w = whereCmd('gcc');
  if (w) { const r = runCmd('"' + w + '" --version', 15000); return { found: true, where: w, ver: r.ran && r.code === 0 ? firstLine(r.out) : '?' }; }
  return { found: false };
}
function chkPython() {
  let r = runCmd('python --version', 25000);
  if (r.ran && r.code === 0 && r.out.indexOf('Python') >= 0) return { found: true, where: whereCmd('python'), ver: firstLine(r.out) };
  r = runCmd('py --version', 25000);
  if (r.ran && r.code === 0) return { found: true, where: 'py launcher', ver: firstLine(r.out) };
  return { found: false };
}
function chkVscode() {
  const la = process.env.LOCALAPPDATA || '';
  const c = chkAppPath('Visual Studio Code', [la + '\\Programs\\Microsoft VS Code\\Code.exe', 'C:\\Program Files\\Microsoft VS Code\\Code.exe'], 'code');
  if (!c.found) { const u = findUninstall('Visual Studio Code'); if (u.found) return { found: true, where: u.name, ver: u.version }; }
  return c;
}
function chkPyCharm() { return chkAppPath('PyCharm', ['C:\\Program Files\\JetBrains\\PyCharm*\\bin\\pycharm64.exe'], null); }
function chkClion() { return chkAppPath('CLion', ['C:\\Program Files\\JetBrains\\CLion*\\bin\\clion64.exe'], null); }
function chkToolbox() {
  const la = process.env.LOCALAPPDATA || '';
  return chkAppPath('JetBrains Toolbox', [la + '\\JetBrains\\Toolbox\\bin\\jetbrains-toolbox.exe'], null);
}
function chkQtCreator() { return chkAppPath('Qt Creator', ['C:\\Qt\\Tools\\QtCreator\\bin\\qtcreator.exe'], 'qtcreator'); }
function chkOffice() {
  const la = process.env.LOCALAPPDATA || '';
  const specs = [
    ['WPS', ['WPS Office', 'Kingsoft'], ['C:\\Program Files (x86)\\Kingsoft\\WPS Office\\*\\office6\\wps.exe', la + '\\Kingsoft\\WPS Office\\*\\office6\\wps.exe']],
    ['QQ', ['QQ', '腾讯QQ'], ['C:\\Program Files (x86)\\Tencent\\QQ\\Bin\\QQ.exe', 'C:\\Program Files\\Tencent\\QQNT\\QQ.exe', 'C:\\Program Files (x86)\\Tencent\\QQNT\\QQ.exe']],
    ['WeChat', ['WeChat', 'Weixin', '微信'], ['C:\\Program Files (x86)\\Tencent\\WeChat\\WeChat.exe', 'C:\\Program Files\\Tencent\\WeChat\\WeChat.exe', 'C:\\Program Files\\Tencent\\Weixin\\Weixin.exe']],
    ['DingTalk', ['DingTalk', '钉钉'], ['C:\\Program Files (x86)\\DingDing\\main\\DingTalk.exe', la + '\\DingTalk\\main\\DingTalk.exe']],
    ['Seewo', ['EasiNote', '希沃'], ['C:\\Program Files (x86)\\seewo\\EasiNote5\\EasiNote5.exe']]
  ];
  return specs.map((s) => {
    const c = chkAppPath(s[0], s[2], null);
    let app = c;
    if (!c.found) { const u = findUninstall(s[1][0]); app = u.found ? { found: true, where: u.name, ver: u.version } : { found: false }; }
    return { name: s[0], found: app.found, ver: app.ver || '?' };
  });
}

// ---------------------------------------------------------------- versions ---
const V = {
  node: 'v22.23.2 (LTS, 适配1709)', mingw: '13.2.0 (UCRT, r8)', git: '2.55.0.5', msys2: '2026-06-11',
  pnpm: '10.x', dsh: '0.1.2-rc.1+', dshnpm: 'npm -g 最新', dshnpx: 'npx 最新', dshgit: '源码 master',
  c1: 'DEV-C++ 5.11.0 + VS Code', c2: 'PyCharm Community + Python 3.13.15 + Toolbox',
  c3: 'CLion + MinGW-w64 13.2 + Toolbox', c4: 'Qt online installer (建议 5.15.x)',
  office: 'WPS/QQ/希沃/钉钉/微信', ai: 'codex + claude code (需 1803+)'
};
const LBL = {
  node: 'itNode', mingw: 'itMingw', git: 'itGit', msys2: 'itMsys2', pnpm: 'itPnpm',
  dsh: 'itDsh', dshnpm: 'itDshNpm', dshnpx: 'itDshNpx', dshgit: 'itDshGit',
  c1: 'itC1', c2: 'itC2', c3: 'itC3', c4: 'itC4', office: 'itOffice', ai: 'itAi'
};

// ---------------------------------------------------------------- install ----
let _lastPct = -1;
function progressPrint(got, total) {
  if (!process.stdout.isTTY) return;
  const pct = total > 0 ? Math.floor((got * 100) / total) : -1;
  if (pct >= 0 && (pct >= _lastPct + 5 || pct >= 100)) {
    _lastPct = pct;
    print('\r  下载中 ' + pct + '%  (' + (got / 1048576).toFixed(1) + ' MB)', 'cyan');
    if (pct >= 100) print('\r\n');
  }
}
function dlDir() {
  const d = path.join(appDir(), 'dl');
  try { fs.mkdirSync(d, { recursive: true }); } catch (e) { /* ignore */ }
  return d;
}
async function downloadTo(L, url, name) {
  const p = path.join(dlDir(), name);
  _lastPct = -1;
  println(L.f('instDl', url), 'yellow');
  try {
    await download(url, p, progressPrint);
    return p;
  } catch (e) {
    println(L.t('instDlBad') + '  [' + e.message + ']', 'red');
    return '';
  }
}
async function verifyPrint(L, label, chkFn) {
  const c = chkFn();
  if (c.found) println(L.f('instVerifyOk', label + '  ' + c.ver), 'green');
  else println(L.f('instVerifyFail', label), 'red');
}
async function runSilent(L, exe, args) {
  println(L.t('instRunSilent'), 'yellow');
  const rc = await runInstall(exe, args, true);
  if (rc === 0) println(L.t('instOk'), 'green');
  else println(L.f('instFail', rc), 'red');
  return rc;
}
function runDetached(L, exe, args) {
  println(L.t('instRunInter'), 'yellow');
  runDetach(exe, args);
}
function fetchText(url) {
  return new Promise((resolve, reject) => {
    const https = require('https'), http = require('http');
    const mod = url.startsWith('https') ? https : http;
    mod.get(url, (res) => {
      let b = '';
      res.on('data', (c) => { b += c; });
      res.on('end', () => resolve(b));
      res.on('error', reject);
    }).on('error', reject);
  });
}
async function installNode(L, online) {
  const name = 'node-v22.23.2-x64.msi';
  let url = 'https://nodejs.org/dist/v22.23.2/' + name;
  let fname = name;
  if (online) {
    try {
      const res = await fetchText('https://nodejs.org/dist/latest-v22.x/SHASUMS256.txt');
      const m = res.match(/node-v22\.[0-9.]+-x64\.msi/);
      if (m) { fname = m[0]; url = 'https://nodejs.org/dist/latest-v22.x/' + fname; }
    } catch (e) { /* keep pinned */ }
  }
  let file;
  if (online) file = await downloadTo(L, url, fname);
  else {
    file = path.join(packagesDir(), name);
    if (!fileExists(file)) { println(L.f('instLocalMissing', name), 'red'); return; }
  }
  if (!file) return;
  await runSilent(L, 'C:\\Windows\\System32\\msiexec.exe', '/i "' + file + '" /qn /norestart');
  await verifyPrint(L, 'Node.js', chkNode);
}
async function installMingw(L, online) {
  const name = 'winlibs-x86_64-posix-seh-gcc-13.2.0-mingw-w64ucrt-11.0.1-r8.zip';
  const url = 'https://github.com/brechtsanders/winlibs_mingw/releases/download/13.2.0posix-18.1.5-11.0.1-ucrt-r8/' + name;
  let file;
  if (online) file = await downloadTo(L, url, name);
  else {
    file = path.join(packagesDir(), name);
    if (!fileExists(file)) { println(L.f('instLocalMissing', name), 'red'); return; }
  }
  if (!file) return;
  const la = process.env.LOCALAPPDATA || '';
  const ps = path.join(appDir(), 'extract-mingw.ps1');
  const src = [
    "$ErrorActionPreference='Stop'",
    "if (-not (Test-Path '" + file + "')) { throw 'zip missing' }",
    "$dest=Join-Path $env:LOCALAPPDATA 'Programs\mingw64'",
    "$tmp=Join-Path $env:LOCALAPPDATA 'Programs\mingw64-tmp'",
    "if(Test-Path $dest){Remove-Item $dest -Recurse -Force}",
    "if(Test-Path $tmp){Remove-Item $tmp -Recurse -Force}",
    "New-Item -ItemType Directory -Force -Path $tmp | Out-Null",
    "Expand-Archive -LiteralPath '" + file + "' -DestinationPath $tmp -Force",
    "$inner=Get-ChildItem $tmp -Directory | Select-Object -First 1",
    "if(-not $inner){ throw 'no dir in zip' }",
    "if($inner.Name -ne 'mingw64'){ Rename-Item $inner.FullName 'mingw64' }",
    "Move-Item (Join-Path $tmp 'mingw64') $dest",
    "Remove-Item $tmp -Recurse -Force",
    "Write-Output 'MINGW_OK'"
  ].join('\r\n');
  writeText(ps, src);
  println(L.t('instRunSilent'), 'yellow');
  const r = runCmd('powershell.exe -NoProfile -ExecutionPolicy Bypass -File "' + ps + '"', 900000);
  if (r.ran && r.out.indexOf('MINGW_OK') >= 0) {
    println(L.t('instOk'), 'green');
    const bin = la + '\\Programs\\mingw64\\bin';
    userPathAdd(bin);
    println(L.t('instPathNote'), 'gray');
  } else {
    println(L.f('instFail', r.code), 'red');
  }
  await verifyPrint(L, 'MinGW-w64', chkMingw);
}
async function installGit(L, online) {
  const name = 'Git-2.55.0.5-64-bit.exe';
  let url = 'https://github.com/git-for-windows/git/releases/download/v2.55.0.windows.5/' + name;
  let fname = name;
  if (online) {
    try {
      const j = JSON.parse(await fetchText('https://api.github.com/repos/git-for-windows/git/releases/latest'));
      const assets = (j.assets || []).filter((a) => /^Git-.*-64-bit\.exe$/.test(a.name));
      if (assets.length) { fname = assets[0].name; url = assets[0].browser_download_url; }
    } catch (e) { /* keep pinned */ }
  }
  let file;
  if (online) file = await downloadTo(L, url, fname);
  else {
    file = path.join(packagesDir(), name);
    if (!fileExists(file)) { println(L.f('instLocalMissing', name), 'red'); return; }
  }
  if (!file) return;
  await runSilent(L, file, '/VERYSILENT /NORESTART /SP- /NOCANCEL');
  await verifyPrint(L, 'Git', chkGit);
}
async function installMsys2(L, online) {
  const name = 'msys2-x86_64-20260611.exe';
  let url = 'https://github.com/msys2/msys2-installer/releases/download/2026-06-11/' + name;
  let fname = name;
  if (online) {
    try {
      const arr = JSON.parse(await fetchText('https://api.github.com/repos/msys2/msys2-installer/releases?per_page=10'));
      for (const rel of arr) {
        const tag = rel.tag_name || '';
        if (tag.indexOf('-') >= 0) continue;
        const assets = rel.assets || [];
        const hit = assets.find((a) => /^msys2-x86_64-.*\.exe$/.test(a.name) && a.name.indexOf('latest') < 0);
        if (hit) { fname = hit.name; url = hit.browser_download_url; break; }
      }
    } catch (e) { /* keep pinned */ }
  }
  let file;
  if (online) file = await downloadTo(L, url, fname);
  else {
    file = path.join(packagesDir(), name);
    if (!fileExists(file)) { println(L.f('instLocalMissing', name), 'red'); return; }
  }
  if (!file) return;
  await runSilent(L, file, '/S');
  const bin = 'C:\\msys64\\usr\\bin';
  userPathAdd(bin);
  println(L.t('instPathNote'), 'gray');
  await verifyPrint(L, 'MSYS2', chkMsys2);
}
function haveNode(L) { if (!chkNode().found) { println(L.t('instNodeMissing'), 'red'); return false; } return true; }
async function installPnpm(L) {
  if (!haveNode(L)) return;
  const r = runCmd('npm install -g pnpm', 600000);
  println(r.ran && r.code === 0 ? L.t('instOk') : L.f('instFail', r.code), r.ran && r.code === 0 ? 'green' : 'red');
  if (r.out.trim()) println(r.out.split(/\r?\n/).slice(-6).join('\r\n'), 'gray');
  await verifyPrint(L, 'pnpm', chkPnpm);
}
async function installDshGlobal(L, preferLocalTgz) {
  if (!haveNode(L)) return;
  let cmdLine = 'npm install -g @deepseek-ai/dsh';
  const pd = packagesDir();
  if (preferLocalTgz) {
    try {
      const tgz = fs.readdirSync(pd).find((f) => /^dsh-.*\.tgz$/.test(f));
      if (tgz) cmdLine = 'npm install -g "' + path.join(pd, tgz) + '"';
    } catch (e) { /* ignore */ }
  }
  const r = runCmd(cmdLine, 900000);
  println(r.ran && r.code === 0 ? L.t('instOk') : L.f('instFail', r.code), r.ran && r.code === 0 ? 'green' : 'red');
  if (r.out.trim()) println(r.out.split(/\r?\n/).slice(-6).join('\r\n'), 'gray');
  await verifyPrint(L, 'Deepseek Harness', chkDsh);
}
function installDshNpx(L) {
  if (!haveNode(L)) return;
  println(L.t('instRunInter'), 'yellow');
  runDetach(process.env.ComSpec || 'C:\\Windows\\System32\\cmd.exe', '/k npx --yes @deepseek-ai/dsh');
}
async function installDshSource(L) {
  if (!haveNode(L)) return;
  if (!chkGit().found) { println(L.t('instGitMissing'), 'red'); return; }
  const ps = path.join(appDir(), 'install-dsh-src.ps1');
  const src = [
    "$ErrorActionPreference='Stop'",
    "$dir=Join-Path $env:LOCALAPPDATA 'TC-tools\dsh-src'",
    "if(!(Test-Path $dir)){ git clone --depth 1 https://github.com/deepseek-ai/deepseek-harness.git $dir | Out-Host }",
    "Push-Location $dir",
    "git pull --ff-only 2>$null",
    "corepack enable 2>$null",
    "corepack prepare pnpm@11.7.0 --activate 2>$null",
    "pnpm install | Out-Host",
    "pnpm run build:official | Out-Host",
    "Push-Location apps\cli",
    "pnpm link --global | Out-Host",
    "Pop-Location",
    "Pop-Location",
    "Write-Output 'DSH_SOURCE_DONE'"
  ].join('\r\n');
  writeText(ps, src);
  println(L.t('instRunSilent'), 'yellow');
  const r = runCmd('powershell.exe -NoProfile -ExecutionPolicy Bypass -File "' + ps + '"', 2700000);
  println(r.ran && r.out.indexOf('DSH_SOURCE_DONE') >= 0 ? L.t('instOk') : L.f('instFail', r.code), r.ran && r.out.indexOf('DSH_SOURCE_DONE') >= 0 ? 'green' : 'red');
  println(r.out.split(/\r?\n/).slice(-8).join('\r\n'), 'gray');
  await verifyPrint(L, 'Deepseek Harness', chkDsh);
}
module.exports = {
  V, LBL,
  __downloadTo: downloadTo,
  chkNode, chkGit, chkPnpm, chkMsys2, chkDsh, chkClaude, chkCodex, chkEdge, chkChrome,
  chkDevCpp, chkMingw, chkPython, chkVscode, chkPyCharm, chkClion, chkToolbox, chkQtCreator, chkOffice,
  installNode, installMingw, installGit, installMsys2, installPnpm,
  installDshGlobal, installDshNpx, installDshSource,
  runSilent, runDetached, verifyPrint
};