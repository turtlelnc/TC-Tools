// core.js - IO / process / registry / system helpers (TC-tools Node.js edition)
'use strict';
const os = require('os');
const path = require('path');
const fs = require('fs');
const http = require('http');
const https = require('https');
const { spawnSync, spawn } = require('child_process');
const readline = require('readline');

// ---------------------------------------------------------------- console ----
const USE_COLOR = process.env.TCTOOL_COLOR === '1' && !!process.stdout.isTTY;
const CD = { def: 37, gray: 90, blue: 94, green: 92, cyan: 96, red: 91, yellow: 93, white: 97 };
function print(s, color) {
  if (USE_COLOR) process.stdout.write('\x1b[' + (CD[color] || 37) + 'm' + s + '\x1b[0m');
  else process.stdout.write(s);
}
function println(s, color) { print(s === undefined ? '' : String(s), color); process.stdout.write('\r\n'); }
function lineSep() { println(' ' + '='.repeat(60), 'cyan'); }

// ---------------------------------------------------------------- strings ----
function fmt(f, ...args) {
  let i = 0;
  return String(f).replace(/%[sd]/g, () => (i < args.length ? String(args[i++]) : ''));
}

// ---------------------------------------------------------------- input ------
// readline.question() stops firing after stdin EOF (piped input), so for non-TTY
// input we use a small manual line reader; TTY keeps readline (editing + echo).
let _rl = null;
let _buf = '';
let _pending = [];
let _ended = false;
let _wired = false;
function wireReader() {
  if (_wired) return;
  _wired = true;
  process.stdin.setEncoding('utf8');
  process.stdin.on('data', (chunk) => {
    _buf += chunk;
    for (;;) {
      const i = _buf.indexOf('\n');
      if (i < 0) break;
      let line = _buf.slice(0, i);
      if (line.endsWith('\r')) line = line.slice(0, -1);
      _buf = _buf.slice(i + 1);
      if (_pending.length) _pending.shift()(line);
      else _bufReserve.push(line);
    }
  });
  process.stdin.on('end', () => {
    _ended = true;
    if (_buf.length > 0) {
      const l = _buf.endsWith('\r') ? _buf.slice(0, -1) : _buf;
      _buf = '';
      if (_pending.length) _pending.shift()(l);
      else _bufReserve.push(l);
    }
    while (_pending.length) _pending.shift()('');
  });
}
const _bufReserve = [];
function readLine() {
  if (process.stdin.isTTY) {
    if (!_rl) _rl = readline.createInterface({ input: process.stdin, output: process.stdout });
    return new Promise((resolve) => { _rl.question('', (a) => resolve(a)); });
  }
  wireReader();
  if (_bufReserve.length > 0) return Promise.resolve(_bufReserve.shift());
  const i = _buf.indexOf('\n');
  if (i >= 0) {
    let line = _buf.slice(0, i);
    if (line.endsWith('\r')) line = line.slice(0, -1);
    _buf = _buf.slice(i + 1);
    return Promise.resolve(line);
  }
  if (_ended) {
    if (_buf.length > 0) { const l = _buf; _buf = ''; return Promise.resolve(l); }
    return Promise.resolve('');
  }
  return new Promise((resolve) => { _pending.push(resolve); });
}
function readLineMasked() {
  return new Promise((resolve) => {
    const stdin = process.stdin;
    if (!stdin.isTTY || typeof stdin.setRawMode !== 'function') { readLine().then(resolve); return; }
    stdin.setRawMode(true); stdin.resume(); stdin.setEncoding('utf8');
    let s = '';
    const onc = (ch) => {
      for (const c of ch) {
        if (c === '\r' || c === '\n') { cleanup(); process.stdout.write('\r\n'); return resolve(s); }
        if (c === '\u0003') { cleanup(); process.stdout.write('\r\n'); return resolve(s); }
        if (c === '\u007f' || c === '\b') { if (s.length) { s = s.slice(0, -1); process.stdout.write('\b \b'); } continue; }
        if (c >= ' ' && c !== '\u001b' && c !== '\t') { s += c; process.stdout.write('*'); }
      }
    };
    const cleanup = () => { stdin.removeListener('data', onc); if (typeof stdin.setRawMode === 'function') stdin.setRawMode(false); stdin.pause(); };
    stdin.on('data', onc);
  });
}
async function kbWait() { await readLine(); }

// ---------------------------------------------------------------- process ----
function runCmd(cmd, timeoutMs) {
  try {
    const r = spawnSync(process.env.ComSpec || 'cmd.exe', ['/c', cmd], { windowsHide: true, timeout: timeoutMs || 25000, maxBuffer: 16 * 1024 * 1024, env: fixEnv() });
    const out = (r.stdout || Buffer.alloc(0)).toString('utf8');
    return { ran: !r.error || !r.error.code, code: (r.status === null ? 1 : r.status), out: out.replace(/[\r\n]+$/, '') };
  } catch (e) {
    return { ran: false, code: 1, out: '' };
  }
}
function firstLine(s) { return String(s).split(/\r?\n/)[0].trim(); }
function fixEnv() {
  // ensure System32 is on PATH (some environments strip it, which breaks where/reg)
  const p = (process.env.Path || process.env.PATH || '');
  if (/\\System32(\\|;|$)/i.test(p)) return process.env;
  const env = Object.assign({}, process.env);
  env.Path = 'C:\\WINDOWS\\System32;' + p;
  env.PATH = env.Path;
  return env;
}
function whereCmd(name) {
  const r = runCmd('where ' + name, 15000);
  if (!r.ran || r.code !== 0) return '';
  return r.out.split(/\r?\n/)[0].trim();
}
function runInstall(exe, args, quiet) {
  return new Promise((resolve) => {
    const cmd = '"' + exe + '" ' + (args || '');
    const child = spawn('cmd.exe', ['/c', cmd], { windowsHide: !!quiet, stdio: 'ignore' });
    let settled = false;
    const ok = (code) => { if (!settled) { settled = true; resolve(code === 0 ? 0 : 1); } };
    child.on('exit', (c) => ok(c));
    child.on('error', async (err) => {
      if (err && (err.code === 'EPERM' || /elevation/i.test(String(err.message)))) {
        const r2 = spawnSync('powershell.exe', ['-NoProfile', '-Command',
          "Start-Process -FilePath '" + exe.replace(/'/g, "''") + "' -ArgumentList '" + String(args || '').replace(/'/g, "''") + "' -Verb RunAs -Wait" + (quiet ? ' -WindowStyle Hidden' : '')], { windowsHide: true });
        ok(r2.status);
      } else ok(1);
    });
  });
}
function runDetach(exe, args) {
  try {
    spinDetach('cmd.exe', ['/c', 'start "" "' + exe + '" ' + (args || '')]);
  } catch (e) { /* ignore */ }
}
function spinDetach(exe, args) {
  const c = spawn(exe, args, { detached: true, stdio: 'ignore', windowsHide: false });
  c.unref();
}
function openUrl(url) {
  try { spinDetach('cmd.exe', ['/c', 'start', '""', url]); } catch (e) { /* ignore */ }
}

// ---------------------------------------------------------------- paths ------
function exeDir() { return path.dirname(__dirname); }
function appDir() {
  const base = process.env.APPDATA || path.join(os.homedir(), 'AppData', 'Roaming');
  const d = path.join(base, 'TC-tools');
  try { fs.mkdirSync(d, { recursive: true }); } catch (e) { /* ignore */ }
  return d;
}
function packagesDir() {
  if (process.env.TCTOOL_PACKAGES) return process.env.TCTOOL_PACKAGES;
  return path.join(appDir(), 'packages');
}
function fileExists(p) { try { return fs.statSync(p).isFile(); } catch (e) { return false; } }
function readText(p) { try { return fs.readFileSync(p, 'utf8'); } catch (e) { return ''; } }
function writeText(p, t) { try { fs.writeFileSync(p, t, 'utf8'); return true; } catch (e) { return false; } }

// ---------------------------------------------------------------- sys --------
function getWinVer() {
  const parts = os.release().split('.').map(Number);
  const build = parts.length > 2 ? parts[2] : 0;
  const map = { 15063: 'Windows 10 1703', 16299: 'Windows 10 1709', 17134: 'Windows 10 1803', 17763: 'Windows 10 1809',
    18362: 'Windows 10 1903', 18363: 'Windows 10 1909', 19041: 'Windows 10 2004', 19042: 'Windows 10 20H2',
    19043: 'Windows 10 21H1', 19044: 'Windows 10 21H2', 19045: 'Windows 10 22H2' };
  let name = map[build];
  if (!name) name = build >= 22000 ? 'Windows 11 (build ' + build + ')' : (build >= 10240 ? 'Windows 10 (build ' + build + ')' : 'Windows (build ' + build + ')');
  return { major: parts[0] || 0, minor: parts[1] || 0, build, name };
}
const _WV = getWinVer();
function winVerName() { return _WV.name; }
function buildAtLeast(b) { return _WV.major > 10 || (_WV.major === 10 && _WV.build >= b); }
function is1709OrOlder() { return !buildAtLeast(17134); }
function osGuard(L) {
  if (!buildAtLeast(15063)) { println(L.f('instOsBlock', winVerName()), 'red'); return false; }
  return true;
}

// ---------------------------------------------------------------- registry ---
let _uninstallCache = null;
function scanUninstall() {
  const roots = [
    'HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall',
    'HKLM\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall',
    'HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall'
  ];
  const apps = [];
  for (const root of roots) {
    const r = runCmd('reg query "' + root + '" /s', 40000);
    if (!r.ran || r.code !== 0) continue;
    let cur = null;
    for (const line of r.out.split(/\r?\n/)) {
      if (/^HKEY_/i.test(line.trim())) { cur = line.trim(); continue; }
      const m = line.match(/^\s+(DisplayName|DisplayVersion)\s+REG_\w+\s+(.+)$/i);
      if (m && cur) {
        let app = apps.find((a) => a.key === cur);
        if (!app) { app = { key: cur, name: '', version: '' }; apps.push(app); }
        if (/^displayname$/i.test(m[1])) app.name = m[2].trim();
        else if (/^displayversion$/i.test(m[1])) app.version = m[2].trim();
      }
    }
  }
  return apps;
}
function uninstallList() { if (!_uninstallCache) _uninstallCache = scanUninstall(); return _uninstallCache; }
function findUninstall(needle) {
  const n = String(needle).toLowerCase();
  const f = uninstallList().find((a) => a.name && a.name.toLowerCase().indexOf(n) >= 0);
  return f ? { found: true, name: f.name, version: f.version || '?' } : { found: false };
}
function userPathAdd(dir) {
  const ps = "& { $p=[Environment]::GetEnvironmentVariable('Path','User'); if($p -notlike '*" + dir + "*'){ [Environment]::SetEnvironmentVariable('Path', ($p.TrimEnd(';') + ';' + '" + dir + "'), 'User') } }";
  runCmd('powershell.exe -NoProfile -Command "' + ps.replace(/"/g, '\"') + '"', 30000);
}
function fileVersionStr(p) {
  try {
    const psFile = path.join(appDir(), 'fv.tmp.ps1');
    writeText(psFile, "(Get-Item '" + p.replace(/'/g, "''") + "').VersionInfo.FileVersion");
    const r = runCmd('powershell.exe -NoProfile -ExecutionPolicy Bypass -File "' + psFile + '"', 20000);
    return r.ran && r.code === 0 ? firstLine(r.out) : '';
  } catch (e) { return ''; }
}

// ---------------------------------------------------------------- net --------
function download(url, filePath, onProgress) {
  return new Promise((resolve, reject) => {
    const u = new URL(url);
    const mod = u.protocol === 'https:' ? https : http;
    const req = mod.get(u, (res) => {
      if ([301, 302, 303, 307, 308].indexOf(res.statusCode) >= 0 && res.headers.location) {
        res.resume();
        download(new URL(res.headers.location, url).href, filePath, onProgress).then(resolve, reject);
        return;
      }
      if (res.statusCode !== 200) { res.resume(); reject(new Error('HTTP ' + res.statusCode)); return; }
      const total = parseInt(res.headers['content-length'] || '0', 10);
      let got = 0;
      res.on('data', (ch) => { got += ch.length; if (onProgress) onProgress(got, total); });
      const ws = fs.createWriteStream(filePath);
      res.pipe(ws);
      ws.on('finish', () => resolve(filePath));
      ws.on('error', reject);
      res.on('error', reject);
    });
    req.on('error', reject);
    req.setTimeout(120000, () => req.destroy(new Error('timeout')));
  });
}
function postStream(url, headers, body, onChunk) {
  return new Promise((resolve, reject) => {
    const u = new URL(url);
    const mod = u.protocol === 'https:' ? https : http;
    const req = mod.request(u, {
      method: 'POST',
      headers: Object.assign({ 'Content-Type': 'application/json', 'Content-Length': Buffer.byteLength(body) }, headers || {})
    }, (res) => {
      res.on('data', (chunk) => { if (onChunk) onChunk(chunk.toString('utf8')); });
      res.on('end', () => resolve(res));
      res.on('error', reject);
    });
    req.on('error', reject);
    req.write(body);
    req.end();
  });
}

// ---------------------------------------------------------------- config ----
function loadConfig() {
  try {
    const t = readText(path.join(appDir(), 'config.json'));
    if (t) return JSON.parse(t);
  } catch (e) { /* ignore */ }
  return {};
}
function saveConfig(cfg) {
  writeText(path.join(appDir(), 'config.json'), JSON.stringify(cfg, null, 2));
}

module.exports = {
  print, println, lineSep, fmt, readLine, readLineMasked, kbWait,
  runCmd, firstLine, whereCmd, runInstall, runDetach, openUrl,
  exeDir, appDir, packagesDir, fileExists, readText, writeText,
  getWinVer, winVerName, buildAtLeast, is1709OrOlder, osGuard,
  findUninstall, userPathAdd, fileVersionStr,
  download, postStream, loadConfig, saveConfig
};