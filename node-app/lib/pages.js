// pages.js - menus & main loop (TC-tools Node.js edition)
'use strict';
const path = require('path');
const langMod = require('./lang');
const core = require('./core');
const tl = require('./tools');
const { cliMain } = require('./cli');
const { print, println, fmt, readLine, kbWait, openUrl, winVerName, buildAtLeast, is1709OrOlder, osGuard,
  findUninstall, userPathAdd, appDir, packagesDir, loadConfig, saveConfig } = core;
const L = langMod.L;

const APP_VERSION = 'v0.1.0-rc2';

async function readChoice(minC, maxC) {
  for (;;) {
    print(curL.t('hintChoose'), 'yellow');
    const s = (await readLine()).trim();
    if (!s) continue;
    const n = parseInt(s, 10);
    if (!isNaN(n) && n >= minC && n <= maxC) return n;
    println(curL.t('invalidChoice'), 'red');
  }
}

function banner() {
  println("  ____  _____  ____   ____            ", "cyan");
  println(" |_   _|_   _|  _ \\ / ___|  _ __  ___  ", "cyan");
  println("   | |   | | | | | | |    | '_ \\/ __| ", "cyan");
  println("   | |   | | | |_| | |___ | | | \\__ \\ ", "cyan");
  println("   |_|   |_| |____/ \\____||_| |_|___/  ", "cyan");
  println("  " + curL.f("version", APP_VERSION) + "  |  Windows 10 1709+", "white");
  println("  " + curL.f("osLine", winVerName()), "gray");
  println("  " + curL.f("langLine", curL.en ? "English" : "简体中文"), "gray");
}

async function pageHome() {
  let exit = false;
  while (!exit) {
    banner();
    println('');
    println(curL.t('homeIntro'), 'yellow');
    println('');
    for (let i = 1; i <= 6; i++) println('  ' + i + '. ' + curL.t('m' + i));
    println('');
    const n = await readChoice(1, 6);
    if (n === 1) await pageInstallMenu();
    else if (n === 2) await pageChecks();
    else if (n === 3) await pageCliPage();
    else if (n === 4) { curL.en = !curL.en; saveConfig({ language: curL.en ? 'en' : 'zh' }); println(curL.f('langSwitched', curL.en ? 'English' : '简体中文'), 'green'); }
    else if (n === 5) { openUrl('https://turtleweb.cc.cd'); println(curL.t('webOpened'), 'green'); }
    else if (n === 6) { println(curL.t('goodbye'), 'green'); exit = true; }
  }
}

const LOCAL_ITEMS = ['node', 'mingw', 'git', 'msys2', 'pnpm', 'dsh', 'c1', 'c2', 'c3', 'c4', 'office', 'ai'];
function itemLine(no, id) {
  println('  ' + no + '. ' + curL.t(tl.LBL[id]) + curL.f('verLabel', tl.V[id]));
}
async function pageInstallMenu() {
  for (;;) {
    core.lineSep();
    println('  ' + curL.t('p1Title'), 'cyan');
    println('');
    println('  1. ' + curL.t('p1Local'));
    println('     ' + curL.t('commentLocal'), 'gray');
    println('  2. ' + curL.t('p1Online'));
    println('     ' + curL.t('commentOnline'), 'gray');
    println('  3. ' + curL.t('back'));
    println('');
    const n = await readChoice(1, 3);
    if (n === 1) await pageLocalInstall();
    else if (n === 2) await pageOnlineInstall();
    else break;
  }
}
async function pageLocalInstall() {
  for (;;) {
    core.lineSep();
    println('  ' + curL.t('p1Title'), 'cyan');
    println('  ' + curL.t('p1Local'), 'white');
    println('  ' + curL.t('commentLocal'), 'gray');
    println('');
    LOCAL_ITEMS.forEach((id, i) => itemLine(i + 1, id));
    println('  ' + (LOCAL_ITEMS.length + 1) + '. ' + curL.t('back'));
    println('');
    const n = await readChoice(1, LOCAL_ITEMS.length + 1);
    if (n === LOCAL_ITEMS.length + 1) break;
    await installItem(LOCAL_ITEMS[n - 1], false);
    await kbWait();
  }
}
async function pageOnlineInstall() {
  const map = { '1': 'node', '2': 'mingw', '3': 'git', '4': 'msys2', '5': 'pnpm', '6': 'c1', '7': 'c2', '8': 'c3', '9': 'c4', '10': 'office', '11': 'ai' };
  for (;;) {
    core.lineSep();
    println('  ' + curL.t('p1Title'), 'cyan');
    println('  ' + curL.t('p1Online'), 'white');
    println('  ' + curL.t('commentOnline'), 'gray');
    println('');
    itemLine(1, 'node'); itemLine(2, 'mingw'); itemLine(3, 'git'); itemLine(4, 'msys2');
    itemLine(5, 'pnpm');
    itemLine('5A', 'dshnpm'); itemLine('5B', 'dshnpx'); itemLine('5C', 'dshgit');
    itemLine(6, 'c1'); itemLine(7, 'c2'); itemLine(8, 'c3'); itemLine(9, 'c4'); itemLine(10, 'office'); itemLine(11, 'ai');
    println('  12. ' + curL.t('back'));
    println('');
    print(curL.t('hintChoose'), 'yellow');
    const s = (await readLine()).trim().toUpperCase();
    let id = null;
    if (s === '5A') id = 'dshnpm';
    else if (s === '5B') id = 'dshnpx';
    else if (s === '5C') id = 'dshgit';
    else if (s === '12') break;
    else id = map[s] || null;
    if (!id) { println(curL.t('invalidChoice'), 'red'); continue; }
    await installItem(id, true);
    await kbWait();
  }
}

// ----------------------------------------------------------------- install --
async function alreadySkip(id, label, chkFn) {
  const c = chkFn();
  if (c.found) { println(curL.f('instAlready', label, c.ver), 'yellow'); return true; }
  return false;
}
async function installItem(id, online) {
  println(curL.f('instStart', curL.t(tl.LBL[id])), 'green');
  if (!osGuard(curL)) return;
  switch (id) {
    case 'node': if (await alreadySkip(id, 'Node.js', tl.chkNode)) return; await tl.installNode(curL, online); break;
    case 'mingw': if (await alreadySkip(id, 'MinGW-w64', tl.chkMingw)) return; await tl.installMingw(curL, online); break;
    case 'git': if (await alreadySkip(id, 'Git', tl.chkGit)) return; await tl.installGit(curL, online); break;
    case 'msys2': if (await alreadySkip(id, 'MSYS2', tl.chkMsys2)) return; await tl.installMsys2(curL, online); break;
    case 'pnpm': if (await alreadySkip(id, 'pnpm', tl.chkPnpm)) return; await tl.installPnpm(curL); break;
    case 'dsh': if (await alreadySkip(id, 'Deepseek Harness', tl.chkDsh)) return; await tl.installDshGlobal(curL, true); break;
    case 'dshnpm': if (await alreadySkip(id, 'Deepseek Harness', tl.chkDsh)) return; await tl.installDshGlobal(curL, false); break;
    case 'dshnpx': if (await alreadySkip(id, 'Deepseek Harness', tl.chkDsh)) return; tl.installDshNpx(curL); break;
    case 'dshgit': if (await alreadySkip(id, 'Deepseek Harness', tl.chkDsh)) return; await tl.installDshSource(curL); break;
    case 'c1': await installBundle1(online); break;
    case 'c2': await installBundle2(online); break;
    case 'c3': await installBundle3(online); break;
    case 'c4': await installBundle4(online); break;
    case 'office': await installOffice(online); break;
    case 'ai': await installAi(); break;
  }
}
async function installBundle1(online) {
  if (!buildAtLeast(15063)) { println(curL.f('instOsBlock', winVerName()), 'red'); return; }
  println('===== ' + curL.t('itC1') + ' =====', 'cyan');
  if (!(await alreadySkip('dev', 'Bloodshed DEV-C++ 5.11.0', tl.chkDevCpp))) await installDevCpp(online);
  if (!(await alreadySkip('vsc', 'VS Code', tl.chkVscode))) await installVscode(online);
  println(curL.t('comboDone'), 'green');
}
async function installBundle2(online) {
  println('===== ' + curL.t('itC2') + ' =====', 'cyan');
  if (!(await alreadySkip('py', 'Python 3 (含 IDLE)', tl.chkPython))) await installPython(online);
  if (!(await alreadySkip('pych', 'PyCharm', tl.chkPyCharm))) await installIdeJetbrains(online, false);
  if (!(await alreadySkip('tb', 'JetBrains Toolbox', tl.chkToolbox))) await installToolbox(online);
  println(curL.t('comboDone'), 'green');
}
async function installBundle3(online) {
  println('===== ' + curL.t('itC3') + ' =====', 'cyan');
  if (!(await alreadySkip('gw', 'MinGW-w64 13.2', tl.chkMingw))) await tl.installMingw(curL, online);
  if (!(await alreadySkip('cl', 'CLion', tl.chkClion))) await installIdeJetbrains(online, true);
  if (!(await alreadySkip('tb2', 'JetBrains Toolbox', tl.chkToolbox))) await installToolbox(online);
  println(curL.t('comboDone'), 'green');
}
async function installBundle4(online) {
  println('===== ' + curL.t('itC4') + ' =====', 'cyan');
  if (!(await alreadySkip('qt', 'Qt Creator', tl.chkQtCreator))) await installQt(online);
  println(curL.t('comboDone'), 'green');
}
async function installDevCpp(online) {
  const name = 'Dev-Cpp 5.11 TDM-GCC 4.9.2 Setup.exe';
  const url = 'https://sourceforge.net/projects/orwelldevcpp/files/Setup%20Releases/Dev-Cpp%205.11%20TDM-GCC%204.9.2%20Setup.exe/download';
  let file;
  if (online) file = await tlDownload(curL, url, name);
  else {
    file = path.join(packagesDir(), name);
    if (!core.fileExists(file)) { println(curL.f('instLocalMissing', name), 'red'); return; }
  }
  if (!file) return;
  await tl.runSilent(curL, file, '/S');
  await tl.verifyPrint(curL, 'DEV-C++', tl.chkDevCpp);
}
async function installVscode(online) {
  const name = 'VSCodeUserSetup-x64-latest.exe';
  const url = 'https://update.code.visualstudio.com/latest/win32-x64-user/stable';
  let file;
  if (online) file = await tlDownload(curL, url, name);
  else {
    file = path.join(packagesDir(), name);
    if (!core.fileExists(file)) { println(curL.f('instLocalMissing', name), 'red'); return; }
  }
  if (!file) return;
  await tl.runSilent(curL, file, '/S');
  await tl.verifyPrint(curL, 'VS Code', tl.chkVscode);
}
async function installPython(online) {
  const name = 'python-3.13.15-amd64.exe';
  const url = 'https://www.python.org/ftp/python/3.13.15/' + name;
  let file;
  if (online) file = await tlDownload(curL, url, name);
  else {
    file = path.join(packagesDir(), name);
    if (!core.fileExists(file)) { println(curL.f('instLocalMissing', name), 'red'); return; }
  }
  if (!file) return;
  await tl.runSilent(curL, file, '/quiet InstallAllUsers=0 PrependPath=1 Include_launcher=1 Include_idle=1 Include_pip=1');
  await tl.verifyPrint(curL, 'Python', tl.chkPython);
}
async function tlDownload(L, url, name) { return require('./tools').__downloadTo(L, url, name); }
async function installIdeJetbrains(online, clion) {
  if (!buildAtLeast(17134)) println(curL.t('instJbNote'), 'yellow');
  const code = clion ? 'CL' : 'PCC';
  const name = clion ? 'CLion-latest.exe' : 'pycharm-community-latest.exe';
  const url = 'https://data.services.jetbrains.com/products/download?code=' + code + '&platform=windows&type=release';
  let file;
  if (online) file = await tlDownload(curL, url, name);
  else {
    file = path.join(packagesDir(), name);
    if (!core.fileExists(file)) { println(curL.f('instLocalMissing', name), 'red'); return; }
  }
  if (!file) return;
  await tl.runSilent(curL, file, '/S');
  await tl.verifyPrint(curL, clion ? 'CLion' : 'PyCharm', clion ? tl.chkClion : tl.chkPyCharm);
}
async function installToolbox(online) {
  const name = 'jetbrains-toolbox-latest.exe';
  const url = 'https://data.services.jetbrains.com/products/download?code=TBA&platform=windows&type=release';
  let file;
  if (online) file = await tlDownload(curL, url, name);
  else {
    file = path.join(packagesDir(), name);
    if (!core.fileExists(file)) { println(curL.f('instLocalMissing', name), 'red'); return; }
  }
  if (!file) return;
  tl.runDetached(curL, file, '');
  await tl.verifyPrint(curL, 'JetBrains Toolbox', tl.chkToolbox);
}
async function installQt(online) {
  const name = 'qt-unified-windows-x64-online.exe';
  const url = 'https://download.qt.io/official_releases/online_installers/' + name;
  let file;
  if (online) file = await tlDownload(curL, url, name);
  else {
    file = path.join(packagesDir(), name);
    if (!core.fileExists(file)) { println(curL.f('instLocalMissing', name), 'red'); return; }
  }
  if (!file) return;
  println(curL.t('instQtNote'), 'gray');
  tl.runDetached(curL, file, '');
  await tl.verifyPrint(curL, 'Qt Creator', tl.chkQtCreator);
}
const OFFICES = [
  ['WPS', 'wps.exe', 'https://www.wps.cn/'],
  ['QQ', 'qq.exe', 'https://im.qq.com/'],
  ['Seewo', 'seewo.exe', 'https://easinote.seewo.com/'],
  ['DingTalk', 'dingtalk.exe', 'https://www.dingtalk.com/'],
  ['WeChat', 'wechat.exe', 'https://pc.weixin.qq.com/']
];
async function installOffice(online) {
  if (online) {
    println(curL.f('instStart', curL.t('itOffice')), 'green');
    for (const o of OFFICES) {
      println(curL.f('instOfflinePage', o[2]), 'yellow');
      openUrl(o[2]);
    }
    println(curL.t('comboDone'), 'green');
    return;
  }
  const missing = [];
  for (const o of OFFICES) {
    const f = path.join(packagesDir(), o[1]);
    if (core.fileExists(f)) {
      println(curL.f('instStart', o[0]), 'green');
      core.runDetach(f, '');
    } else missing.push(o[1]);
  }
  if (missing.length) println(curL.f('instLocalMissing', missing.join('; ')), 'red');
  println(curL.t('comboDone'), 'green');
}
async function installAi() {
  if (is1709OrOlder()) { println(curL.f('instAiBlock', core.getWinVer().build), 'red'); return; }
  if (!tl.chkNode().found) { println(curL.t('instNodeMissing'), 'red'); return; }
  const r = core.runCmd('npm install -g @openai/codex @anthropic-ai/claude-code', 900000);
  println(r.ran && r.code === 0 ? curL.t('instOk') : curL.f('instFail', r.code), r.ran && r.code === 0 ? 'green' : 'red');
  if (r.out.trim()) println(r.out.split(/\r?\n/).slice(-8).join('\r\n'), 'gray');
  const c1 = tl.chkCodex(), c2 = tl.chkClaude();
  if (c1.found) println(curL.f('installed', 'codex ' + c1.ver), 'green');
  if (c2.found) println(curL.f('installed', 'claude code ' + c2.ver), 'green');
}

// ----------------------------------------------------------------- checks --
async function pageChecks() {
  for (;;) {
    core.lineSep();
    println('  ' + curL.t('p2Title'), 'cyan');
    println('');
    for (let i = 1; i <= 16; i++) println('  ' + i + '. ' + curL.t('c' + i));
    println('');
    const n = await readChoice(1, 16);
    if (n === 1) await runCheckSummary();
    else if (n === 16) break;
    else await runCheckItem(n);
  }
}
function printFound(name, c) {
  if (c.found) {
    println(curL.f('chkVer', c.ver || '?'), 'green');
    if (c.where) println(curL.f('chkPath', c.where), 'gray');
  } else {
    println(curL.f('chkNotFound', name), 'red');
  }
}
async function runCheckSummary() {
  core.lineSep();
  println(curL.t('chkSummaryHead'), 'cyan');
  const list = [['Node.js', tl.chkNode], ['Git', tl.chkGit], ['pnpm', tl.chkPnpm], ['MSYS2', tl.chkMsys2],
    ['Deepseek Harness', tl.chkDsh], ['Claude Code', tl.chkClaude], ['Codex', tl.chkCodex],
    ['Microsoft Edge', tl.chkEdge], ['Google Chrome', tl.chkChrome], ['DEV-C++', tl.chkDevCpp],
    ['MinGW-w64', tl.chkMingw], ['Python', tl.chkPython], ['VS Code', tl.chkVscode],
    ['PyCharm', tl.chkPyCharm], ['CLion', tl.chkClion], ['JetBrains Toolbox', tl.chkToolbox],
    ['Qt Creator', tl.chkQtCreator]];
  let found = 0;
  for (const p of list) {
    const c = p[1]();
    if (c.found) { found++; println('  [OK] ' + p[0] + '  ' + c.ver, 'green'); }
    else println('  [X ] ' + p[0], 'red');
  }
  for (const o of tl.chkOffice()) {
    if (o.found) { found++; println('  [OK] ' + o.name + '  ' + o.ver, 'green'); }
    else println('  [X ] ' + o.name, 'red');
  }
  println(curL.f('chkSummaryTotal', found), 'gray');
  await kbWait();
}
async function runCheckItem(n) {
  core.lineSep();
  println(curL.f('chkHeader', curL.t('c' + n)), 'cyan');
  switch (n) {
    case 2: { const c = tl.chkNode(); printFound('Node.js', c); if (c.found) { const nv = core.runCmd('npm --version', 20000); println(nv.ran && nv.code === 0 ? curL.f('chkVer', 'npm ' + nv.out) : curL.f('chkError', 'npm'), nv.ran && nv.code === 0 ? 'green' : 'red'); } else println(curL.t('tipNode'), 'yellow'); break; }
    case 3: { const c = tl.chkGit(); printFound('Git', c); if (!c.found) println(curL.t('tipGit'), 'yellow'); break; }
    case 4: { const c = tl.chkPnpm(); printFound('pnpm', c); if (!c.found) println(curL.t('tipPnpm'), 'yellow'); break; }
    case 5: { const c = tl.chkMsys2(); printFound('MSYS2', c); if (!c.found) println(curL.t('tipMsys2'), 'yellow'); break; }
    case 6: { const c = tl.chkDsh(); printFound('Deepseek Harness (dsh)', c); if (!c.found) println(curL.t('tipDsh'), 'yellow'); break; }
    case 7: { const c = tl.chkClaude(); printFound('Claude Code', c); if (!c.found) println(curL.t('tipClaude'), 'yellow'); break; }
    case 8: { const c = tl.chkCodex(); printFound('Codex', c); if (!c.found) println(curL.t('tipCodex'), 'yellow'); break; }
    case 9: { const c = tl.chkEdge(); printFound('Microsoft Edge', c); if (!c.found) println(curL.t('tipEdge'), 'yellow'); break; }
    case 10: { const c = tl.chkChrome(); printFound('Google Chrome', c); if (!c.found) println(curL.t('tipChrome'), 'yellow'); break; }
    case 11: { const c = tl.chkDevCpp(); printFound('Bloodshed DEV-C++ 5.11.0', c); if (!c.found) println(curL.t('tipDevCpp'), 'yellow'); break; }
    case 12: { const c = tl.chkMingw(); printFound('MinGW-w64 (gcc)', c); if (!c.found) println(curL.t('tipMingw'), 'yellow'); break; }
    case 13: { const c = tl.chkPython(); printFound('Python 3', c); if (!c.found) println(curL.t('tipPython'), 'yellow'); break; }
    case 14: {
      const list = [['VS Code', tl.chkVscode], ['PyCharm', tl.chkPyCharm], ['CLion', tl.chkClion],
        ['JetBrains Toolbox', tl.chkToolbox], ['Qt Creator', tl.chkQtCreator], ['DEV-C++', tl.chkDevCpp]];
      for (const p of list) printFound(p[0], p[1]());
      println(curL.t('tipIde'), 'yellow');
      break;
    }
    case 15: {
      for (const o of tl.chkOffice()) {
        if (o.found) println(curL.f('chkVer', o.name + ' ' + o.ver), 'green');
        else println(curL.f('chkNotFound', o.name), 'red');
      }
      break;
    }
  }
  await kbWait();
}
async function pageCliPage() {
  for (;;) {
    core.lineSep();
    println('  ' + curL.t('p3Title'), 'cyan');
    println('');
    println(curL.t('p3Desc1'), 'gray');
    println(curL.t('p3Desc2'), 'gray');
    println(curL.t('p3Desc3'), 'gray');
    println('');
    println('  1. ' + curL.t('p3Start'), 'white');
    println('  2. ' + curL.t('back'));
    println('');
    const n = await readChoice(1, 2);
    if (n === 2) break;
    await cliMain(curL);
  }
}

let curL = null;
function main() {
  const cfg = loadConfig();
  curL = new L(cfg.language === 'en');
  if (!buildAtLeast(15063)) {
    println(curL.f('osUnsupported', winVerName()), 'red');
    process.exit(1);
  }
  pageHome().then(() => { process.exit(0); }).catch((e) => {
    println(curL.f('cliErrHttp', String((e && e.message) || e)), 'red');
    process.exit(1);
  });
}
module.exports = { main, runCheckSummary, _setLang: (en) => { curL = new L(en); } };