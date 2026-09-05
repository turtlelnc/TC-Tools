// tools.cpp - part1: checks
#include "tools.hpp"

#include <windows.h>

#include <cstring>
#include <string>
#include <vector>

using namespace tcu;
using namespace tools;

// ---- helpers ----
static std::string firstLine(const std::string& s) {
  std::string t = trim(s);
  size_t n = t.find('\n');
  if (n != std::string::npos) t = t.substr(0, n);
  n = t.find('\r');
  if (n != std::string::npos) t = t.substr(0, n);
  return trim(t);
}

static std::string firstPath(const std::string& s) {
  std::string t = trim(s);
  size_t n = t.find('\n');
  if (n != std::string::npos) t = t.substr(0, n);
  return trim(t);
}

static bool globMatch(const std::string& wcPattern, std::string& outPath) {
  std::wstring wp = u8w(wcPattern);
  size_t slash = wcPattern.find_last_of("\\/");
  std::string dir = slash == std::string::npos ? "" : wcPattern.substr(0, slash + 1);
  WIN32_FIND_DATAW fd;
  HANDLE h = FindFirstFileW(wp.c_str(), &fd);
  if (h == INVALID_HANDLE_VALUE) return false;
  FindClose(h);
  outPath = dir + wu8(fd.cFileName);
  return true;
}

static std::string envExpand(const std::string& in) {
  std::string out;
  std::string cur = in;
  for (;;) {
    size_t a = cur.find('%');
    if (a == std::string::npos) { out += cur; break; }
    size_t b = cur.find('%', a + 1);
    out += cur.substr(0, a);
    if (b == std::string::npos) { out += cur.substr(a); break; }
    std::string name = cur.substr(a + 1, b - a - 1);
    wchar_t buf[MAX_PATH * 4]; DWORD n = GetEnvironmentVariableW(u8w(name).c_str(), buf, MAX_PATH * 4);
    if (n > 0) out += wu8(std::wstring(buf, n));
    out += cur.substr(b + 1);
    cur = out; out.clear();
    // keep scanning remaining after the expanded value
    break;
  }
  return cur.empty() ? in : cur;
}

static Chk chkFromCmd(const std::string& displayName, const std::string& whereName, const std::string& verArgs) {
  Chk c;
  c.found = false;
  std::string w = firstPath(whereCmd(whereName));
  if (w.empty()) return c;
  c.found = true;
  c.where = w;
  bool script = w.size() > 4 && (w.compare(w.size() - 4, 4, ".cmd") == 0 ||
                                 w.compare(w.size() - 4, 4, ".bat") == 0 ||
                                 w.compare(w.size() - 4, 4, ".ps1") == 0);
  std::string vc = script ? ("cmd /c \"" + w + "\" " + verArgs) : ("\"" + w + "\" " + verArgs);
  auto r = runCmd(vc, 20000);
  if (r.ran && r.code == 0 && !trim(r.out).empty()) c.ver = firstLine(r.out);
  else c.ver = "?";
  return c;
}

// ---- tool checks ----
Chk tools::chkNode() {
  Chk c = chkFromCmd("Node.js", "node", "--version");
  return c;
}
Chk tools::chkGit() {
  Chk c = chkFromCmd("Git", "git", "--version");
  return c;
}
Chk tools::chkPnpm() {
  Chk c = chkFromCmd("pnpm", "pnpm", "--version");
  return c;
}

Chk tools::chkMsys2() {
  Chk c;
  std::string bash = "C:\\msys64\\usr\\bin\\bash.exe";
  if (fileExists(bash)) {
    c.found = true; c.where = bash;
    auto r = runCmd("\"" + bash + "\" --version", 15000);
    if (r.ran) c.ver = firstLine(r.out);
    return c;
  }
  const char* p[] = { "C:\\msys64\\usr\\bin\\bash.exe", "C:\\msys64\\usr\\bin\\msys-2.0.dll" };
  (void)p;
  FoundApp u = findUninstall("MSYS2");
  if (u.found) { c.found = true; c.where = u.name; c.ver = u.version.empty() ? "MSYS2" : u.version; }
  else {
    std::string w = firstPath(whereCmd("bash"));
    if (!w.empty()) { c.found = true; c.where = w; c.ver = "found in PATH"; }
  }
  return c;
}

Chk tools::chkDsh() {
  Chk c;
  std::string w = firstPath(whereCmd("dsh"));
  if (w.empty()) return c;
  c.found = true; c.where = w;
  auto r = runCmd("cmd /c dsh --version", 45000);
  if (r.ran && r.code == 0) c.ver = firstLine(r.out);
  return c;
}

Chk tools::chkClaude() {
  Chk c = chkFromCmd("Claude Code", "claude", "--version");
  return c;
}
Chk tools::chkCodex() {
  Chk c = chkFromCmd("Codex", "codex", "--version");
  return c;
}

static Chk chkAppPath(const std::string& name, const std::vector<std::string>& paths, const char* whereName) {
  Chk c;
  for (auto& p : paths) {
    std::string m;
    if (p.find('*') != std::string::npos) { if (globMatch(p, m)) { c.found = true; c.where = m; c.ver = fileVersionStr(m); break; } }
    else if (fileExists(p)) { c.found = true; c.where = p; c.ver = fileVersionStr(p); break; }
  }
  if (!c.found && whereName) {
    std::string w = firstPath(whereCmd(whereName));
    if (!w.empty()) { c.found = true; c.where = w; }
  }
  if (!c.found) {
    FoundApp u = findUninstall(name);
    if (u.found) { c.found = true; c.where = u.name; c.ver = u.version; }
  }
  if (c.ver.empty() && c.found) c.ver = "?";
  return c;
}

Chk tools::chkEdge() {
  std::string appdata = envExpand("%LOCALAPPDATA%");
  std::vector<std::string> paths = {
    "C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe",
    "C:\\Program Files\\Microsoft\\Edge\\Application\\msedge.exe",
    "C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\MicrosoftEdge.exe"
  };
  (void)appdata;
  Chk c = chkAppPath("Microsoft Edge", paths, nullptr);
  return c;
}

Chk tools::chkChrome() {
  std::vector<std::string> paths = {
    "C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe",
    "C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe",
    envExpand("%LOCALAPPDATA%") + "\\Google\\Chrome\\Application\\chrome.exe"
  };
  Chk c = chkAppPath("Google Chrome", paths, nullptr);
  return c;
}

Chk tools::chkDevCpp() {
  std::vector<std::string> paths = { "C:\\Program Files (x86)\\Dev-Cpp\\devcpp.exe", "C:\\Dev-Cpp\\devcpp.exe" };
  Chk c = chkAppPath("Dev-C++", paths, nullptr);
  if (!c.found && !c.ver.empty()) c.found = true;
  if (!c.found) { FoundApp u = findUninstall("Dev-C++"); if (u.found) { c.found = true; c.where = u.name; c.ver = u.version; } }
  return c;
}

Chk tools::chkMingw() {
  Chk c;
  std::vector<std::string> cands = {
    "C:\\mingw64\\bin\\gcc.exe",
    envExpand("%LOCALAPPDATA%") + "\\Programs\\mingw64\\bin\\gcc.exe",
    "C:\\msys64\\mingw64\\bin\\gcc.exe",
    "C:\\msys64\\usr\\bin\\gcc.exe"
  };
  for (auto& p : cands) {
    if (fileExists(p)) {
      c.found = true; c.where = p;
      auto r = runCmd("\"" + p + "\" --version", 15000);
      if (r.ran && r.code == 0) c.ver = firstLine(r.out);
      return c;
    }
  }
  std::string w = firstPath(whereCmd("gcc"));
  if (!w.empty()) {
    c.found = true; c.where = w;
    auto r = runCmd("\"" + w + "\" --version", 15000);
    if (r.ran && r.code == 0) c.ver = firstLine(r.out);
  }
  return c;
}

Chk tools::chkPython() {
  Chk c;
  auto r = runCmd("python --version", 20000);
  if (r.ran && r.code == 0 && contains(r.out, "Python")) {
    c.found = true; c.ver = firstLine(r.out);
    std::string w = firstPath(whereCmd("python"));
    c.where = w;
  } else {
    auto r2 = runCmd("py --version", 20000);
    if (r2.ran && r2.code == 0) { c.found = true; c.ver = firstLine(r2.out); c.where = "py launcher"; }
  }
  return c;
}

Chk tools::chkVscode() {
  Chk c;
  std::string appdata = envExpand("%LOCALAPPDATA%");
  std::vector<std::string> paths = {
    appdata + "\\Programs\\Microsoft VS Code\\Code.exe",
    "C:\\Program Files\\Microsoft VS Code\\Code.exe"
  };
  c = chkAppPath("Visual Studio Code", paths, "code");
  if (!c.found) { FoundApp u = findUninstall("Visual Studio Code"); if (u.found) { c.found = true; c.where = u.name; c.ver = u.version; } }
  return c;
}

Chk tools::chkPyCharm() {
  std::vector<std::string> paths = {
    "C:\\Program Files\\JetBrains\\PyCharm*\\bin\\pycharm64.exe",
    envExpand("%LOCALAPPDATA%") + "\\Programs\\PyCharm*\\bin\\pycharm64.exe",
    "C:\\Program Files\\JetBrains\\PyCharm*\\bin\\pycharm.exe"
  };
  return chkAppPath("PyCharm", paths, nullptr);
}

Chk tools::chkClion() {
  std::vector<std::string> paths = {
    "C:\\Program Files\\JetBrains\\CLion*\\bin\\clion64.exe",
    envExpand("%LOCALAPPDATA%") + "\\Programs\\CLion*\\bin\\clion64.exe",
    "C:\\Program Files\\JetBrains\\CLion*\\bin\\clion.exe"
  };
  return chkAppPath("CLion", paths, nullptr);
}

Chk tools::chkToolbox() {
  std::vector<std::string> paths = {
    envExpand("%LOCALAPPDATA%") + "\\JetBrains\\Toolbox\\bin\\jetbrains-toolbox.exe",
    "C:\\Program Files\\JetBrains\\Toolbox\\bin\\jetbrains-toolbox.exe"
  };
  return chkAppPath("JetBrains Toolbox", paths, nullptr);
}

Chk tools::chkQtCreator() {
  std::vector<std::string> paths = {
    "C:\\Qt\\Tools\\QtCreator\\bin\\qtcreator.exe",
    envExpand("%LOCALAPPDATA%") + "\\Qt\\Tools\\QtCreator\\bin\\qtcreator.exe"
  };
  return chkAppPath("Qt Creator", paths, "qtcreator");
}

std::vector<OfficeChk> tools::chkOffice() {
  std::vector<OfficeChk> out;
  struct Spec { std::string name; std::vector<std::string> needle; std::vector<std::string> paths; };
  std::string la = envExpand("%LOCALAPPDATA%");
  std::vector<Spec> specs = {
    { "WPS", { "WPS Office", "Kingsoft" },
      { "C:\\Program Files (x86)\\Kingsoft\\WPS Office\\*\\office6\\wps.exe",
        la + "\\Kingsoft\\WPS Office\\*\\office6\\wps.exe",
        "C:\\Users\\*\\AppData\\Local\\Kingsoft\\WPS Office\\*\\office6\\wps.exe" } },
    { "QQ", { "QQ", "腾讯QQ" },
      { "C:\\Program Files (x86)\\Tencent\\QQ\\Bin\\QQ.exe",
        "C:\\Program Files\\Tencent\\QQNT\\QQ.exe",
        "C:\\Program Files (x86)\\Tencent\\QQNT\\QQ.exe" } },
    { "WeChat", { "WeChat", "Weixin", "微信" },
      { "C:\\Program Files (x86)\\Tencent\\WeChat\\WeChat.exe",
        "C:\\Program Files\\Tencent\\WeChat\\WeChat.exe",
        "C:\\Program Files\\Tencent\\Weixin\\Weixin.exe",
        la + "\\Tencent\\WeChat\\WeChat.exe" } },
    { "DingTalk", { "DingTalk", "钉钉" },
      { "C:\\Program Files (x86)\\DingDing\\main\\DingTalk.exe",
        la + "\\DingTalk\\main\\DingTalk.exe" } },
    { "Seewo", { "EasiNote", "希沃" },
      { "C:\\Program Files (x86)\\seewo\\EasiNote5\\EasiNote5.exe",
        "C:\\Program Files\\seewo\\EasiNote5\\EasiNote5.exe" } }
  };
  for (auto& s : specs) {
    OfficeChk o; o.name = s.name;
    for (auto& p : s.paths) {
      std::string m;
      if (p.find('*') != std::string::npos) { if (globMatch(p, m)) { o.found = true; o.ver = fileVersionStr(m); break; } }
      else if (fileExists(p)) { o.found = true; o.ver = fileVersionStr(p); break; }
    }
    if (!o.found) for (auto& nd : s.needle) {
      FoundApp u = findUninstall(nd);
      if (u.found) { o.found = true; o.ver = u.version; break; }
    }
    if (o.ver.empty() && o.found) o.ver = "?";
    out.push_back(o);
  }
  return out;
}
// tools.cpp - part2: install core + core tools
#include "tools.hpp"
#include "http.hpp"
#include "json.hpp"

#include <windows.h>

#include <functional>
#include <string>
#include <vector>

using namespace tcu;
using namespace tools;

static bool osGuard(App& a) {
  if (!buildAtLeast(15063)) {
    println(a.f(LK_INST_OS_BLOCK, a.wv.name.c_str()), CLR_RED);
    kbWait();
    return false;
  }
  return true;
}

static std::string dlDir() {
  std::string d = appDir() + "\\dl";
  CreateDirectoryW(u8w(d).c_str(), nullptr);
  return d;
}

static int g_lastPct = -1;
static void progressPrint(App& a, long long got, long long total) {
  (void)a;
  if (!isConsole()) return;
  int pct = total > 0 ? (int)(got * 100 / total) : -1;
  if (pct >= 0 && (pct >= g_lastPct + 5 || pct >= 100)) {
    g_lastPct = pct;
    double mb = got / 1048576.0;
    print(sf("\r  下载中 %3d%%  (%.1f MB)", pct, mb), CLR_CYAN);
    if (pct >= 100) print("\r\n", CLR_DEF);
  }
}

static std::string downloadTo(App& a, const std::string& url, const std::string& name) {
  std::string p = dlDir() + "\\" + name;
  g_lastPct = -1;
  println(a.f(LK_INST_DL, url.c_str()), CLR_YELLOW);
  std::string err;
  bool ok = tch::download(url, p, {}, [&](long long got, long long total) { progressPrint(a, got, total); }, &err);
  if (!isConsole()) print("\r\n", CLR_DEF);
  if (!ok) {
    println(a.tr(LK_INST_DL_BAD) + "  [" + err + "]", CLR_RED);
    return "";
  }
  return p;
}

static int runSilent(App& a, const std::wstring& exe, const std::wstring& args) {
  println(a.tr(LK_INST_RUN_SILENT), CLR_YELLOW);
  int rc = runInstallExe(exe, args, true);
  if (rc == 0) println(a.tr(LK_INST_OK), CLR_GREEN);
  else if (rc == 2) println(a.tr(LK_INST_CANCEL), CLR_RED);
  else println(a.f(LK_INST_FAIL, rc), CLR_RED);
  return rc;
}

static void runDetached(App& a, const std::wstring& exe, const std::wstring& args) {
  println(a.tr(LK_INST_RUN_INTER), CLR_YELLOW);
  runDetach(exe, args);
}

static void verifyPrint(App& a, const std::string& label, const std::function<Chk()>& chk) {
  Chk c = chk();
  if (c.found) println(a.f(LK_INST_VERIFY_OK, (label + "  " + c.ver).c_str()), CLR_GREEN);
  else println(a.f(LK_INST_VERIFY_FAIL, label.c_str()), CLR_RED);
}

static void showCmdTail(const CmdResult& r, int lines) {
  std::string out = trim(r.out);
  if (out.empty() && r.code != 0) { println(sf("(exit %d)", r.code), CLR_GRAY); return; }
  // last N lines
  std::vector<std::string> ls;
  size_t pos = 0;
  while (pos <= out.size()) {
    size_t n = out.find('\n', pos);
    if (n == std::string::npos) { ls.push_back(out.substr(pos)); break; }
    ls.push_back(out.substr(pos, n - pos));
    pos = n + 1;
  }
  size_t start = ls.size() > (size_t)lines ? ls.size() - lines : 0;
  for (size_t i = start; i < ls.size(); ++i) print(trim(ls[i]) + "\r\n", CLR_GRAY);
}

// find an asset in a GitHub release JSON
static std::string ghAsset(const std::string& json, const std::function<bool(const std::string&)>& match, std::string* tag = nullptr) {
  Json j;
  if (!Json::parse(json, j) || !j.isObj()) return "";
  if (tag) *tag = j.str("tag_name");
  const Json* assets = j.get("assets");
  if (!assets || assets->t != Json::ARR) return "";
  for (auto& a : assets->arr) {
    std::string n = a.str("name");
    if (match(n)) return a.str("browser_download_url");
  }
  return "";
}

// ---------------------------------------------------------------- Node.js
static void installNode(App& a, bool online) {
  std::string name = "node-v22.23.2-x64.msi";
  std::string url = "https://nodejs.org/dist/v22.23.2/" + name;
  if (online) {
    HttpResp resp; std::string err;
    if (tch::getText("https://nodejs.org/dist/latest-v22.x/SHASUMS256.txt", resp, {}, &err) && resp.status == 200) {
      size_t p = resp.body.find("node-v22.");
      if (p != std::string::npos) {
        size_t e = resp.body.find("-x64.msi", p);
        if (e != std::string::npos) {
          name = resp.body.substr(p, e - p + 8);
          url = "https://nodejs.org/dist/latest-v22.x/" + name;
        }
      }
    }
  }
  std::string file;
  if (online) file = downloadTo(a, url, name);
  else {
    file = packagesDir() + "\\" + name;
    if (!fileExists(file)) { println(a.f(LK_INST_LOCAL_MISSING, name.c_str()), CLR_RED); return; }
  }
  if (file.empty()) return;
  runSilent(a, L"C:\\Windows\\System32\\msiexec.exe", L"/i \"" + u8w(file) + L"\" /qn /norestart");
  verifyPrint(a, "Node.js", chkNode);
}

// ---------------------------------------------------------------- MinGW-w64 (WinLibs 13.2.0)
static void installMingw(App& a, bool online) {
  std::string name = "winlibs-x86_64-posix-seh-gcc-13.2.0-mingw-w64ucrt-11.0.1-r8.zip";
  std::string url = "https://github.com/brechtsanders/winlibs_mingw/releases/download/13.2.0posix-18.1.5-11.0.1-ucrt-r8/" + name;
  std::string file;
  if (online) file = downloadTo(a, url, name);
  else {
    file = packagesDir() + "\\" + name;
    if (!fileExists(file)) { println(a.f(LK_INST_LOCAL_MISSING, name.c_str()), CLR_RED); return; }
  }
  if (file.empty()) return;
  // extract via powershell
  std::string ps = appDir() + "\\extract-mingw.ps1";
  std::string psrc;
  psrc += "\r$ErrorActionPreference='Stop'\r\r\n";
  psrc += "\rif (-not (Test-Path '" + file + "')) { throw 'zip missing' }\r\r\n";
  psrc += "\r$dest=Join-Path $env:LOCALAPPDATA 'Programs\\mingw64'\r\r\n";
  psrc += "\r$tmp=Join-Path $env:LOCALAPPDATA 'Programs\\mingw64-tmp'\r\r\n";
  psrc += "\rif(Test-Path $dest){Remove-Item $dest -Recurse -Force}\r\r\n";
  psrc += "\rif(Test-Path $tmp){Remove-Item $tmp -Recurse -Force}\r\r\n";
  psrc += "\rNew-Item -ItemType Directory -Force -Path $tmp | Out-Null\r\r\n";
  psrc += "\rExpand-Archive -LiteralPath '" + file + "' -DestinationPath $tmp -Force\r\r\n";
  psrc += "\r$inner=Get-ChildItem $tmp -Directory | Select-Object -First 1\r\r\n";
  psrc += "\rif(-not $inner){ throw 'no dir in zip' }\r\r\n";
  psrc += "\rif($inner.Name -ne 'mingw64'){ Rename-Item $inner.FullName 'mingw64' }\r\r\n";
  psrc += "\rMove-Item (Join-Path $tmp 'mingw64') $dest\r\r\n";
  psrc += "\rRemove-Item $tmp -Recurse -Force\r\r\n";
  psrc += "\rWrite-Output 'MINGW_OK'\r\r\n";
  writeTextFile(ps, psrc);
  println(a.tr(LK_INST_RUN_SILENT), CLR_YELLOW);
  CmdResult r = runCmd("powershell.exe -NoProfile -ExecutionPolicy Bypass -File \"" + ps + "\"", 900000);
  if (r.ran && contains(r.out, "MINGW_OK")) {
    println(a.tr(LK_INST_OK), CLR_GREEN);
    std::string bin = envExpand("%LOCALAPPDATA%") + "\\Programs\\mingw64\\bin";
    if (!userPathHas(bin)) { userPathAdd(bin); println(a.tr(LK_INST_PATH_NOTE), CLR_GRAY); }
  } else {
    println(a.f(LK_INST_FAIL, r.code), CLR_RED);
    print(trim(r.out).substr(0, 400) + "\r\n", CLR_GRAY);
  }
  verifyPrint(a, "MinGW-w64", chkMingw);
}

// ---------------------------------------------------------------- Git/ ---------------------------------------------------------------- Git
static void installGit(App& a, bool online) {
  std::string name = "Git-2.55.0.5-64-bit.exe";
  std::string url = "https://github.com/git-for-windows/git/releases/download/v2.55.0.windows.5/" + name;
  if (online) {
    HttpResp resp; std::string err;
    if (tch::getText("https://api.github.com/repos/git-for-windows/git/releases/latest", resp,
                     { "User-Agent: TC-tools/0.1.0-rc1" }, &err) && resp.status == 200) {
      std::string u = ghAsset(resp.body, [](const std::string& n) {
        return n.size() > 12 && startsWith(n, "Git-") && n.find("-64-bit.exe") != std::string::npos;
      });
      if (!u.empty()) { url = u; name = url.substr(url.find_last_of('/') + 1); }
    }
  }
  std::string file;
  if (online) file = downloadTo(a, url, name);
  else {
    file = packagesDir() + "\\" + name;
    if (!fileExists(file)) { println(a.f(LK_INST_LOCAL_MISSING, name.c_str()), CLR_RED); return; }
  }
  if (file.empty()) return;
  runSilent(a, u8w(file), L"/VERYSILENT /NORESTART /SP- /NOCANCEL");
  verifyPrint(a, "Git", chkGit);
}

// ---------------------------------------------------------------- MSYS2
static void installMsys2(App& a, bool online) {
  std::string name = "msys2-x86_64-20260611.exe";
  std::string url = "https://github.com/msys2/msys2-installer/releases/download/2026-06-11/" + name;
  if (online) {
    HttpResp resp; std::string err;
    if (tch::getText("https://api.github.com/repos/msys2/msys2-installer/releases?per_page=10", resp,
                     { "User-Agent: TC-tools/0.1.0-rc1" }, &err) && resp.status == 200) {
      Json j;
      if (Json::parse(resp.body, j) && j.t == Json::ARR) {
        for (auto& rel : j.arr) {
          std::string tag = rel.str("tag_name");
          if (tag.find('-') != std::string::npos) continue; // skip nightly-like
          const Json* assets = rel.get("assets");
          if (!assets) continue;
          for (auto& as : assets->arr) {
            std::string an = as.str("name");
            if (an.find("msys2-x86_64-") == 0 && (an.size() > 4 && an.compare(an.size() - 4, 4, ".exe") == 0) && an.find("latest") == std::string::npos) {
              name = an;
              url = as.str("browser_download_url");
              goto found;
            }
          }
        }
      }
    }
  found:;
  }
  std::string file;
  if (online) file = downloadTo(a, url, name);
  else {
    file = packagesDir() + "\\" + name;
    if (!fileExists(file)) { println(a.f(LK_INST_LOCAL_MISSING, name.c_str()), CLR_RED); return; }
  }
  if (file.empty()) return;
  runSilent(a, u8w(file), L"/S");
  std::string bin = "C:\\msys64\\usr\\bin";
  if (!userPathHas(bin)) { userPathAdd(bin); println(a.tr(LK_INST_PATH_NOTE), CLR_GRAY); }
  verifyPrint(a, "MSYS2", chkMsys2);
}

// ---------------------------------------------------------------- pnpm / dsh
static bool haveNode(App& a) {
  Chk c = chkNode();
  if (!c.found) { println(a.tr(LK_INST_NODE_MISSING), CLR_RED); return false; }
  return true;
}

static void installPnpm(App& a, bool online) {
  (void)online;
  if (!haveNode(a)) return;
  CmdResult r = runCmd("npm install -g pnpm", 600000);
  if (r.ran && r.code == 0) { println(a.tr(LK_INST_OK), CLR_GREEN); }
  else { println(a.f(LK_INST_FAIL, r.code), CLR_RED); }
  showCmdTail(r, 6);
  verifyPrint(a, "pnpm", chkPnpm);
}

static void installDshGlobal(App& a, bool preferLocalTgz) {
  if (!haveNode(a)) return;
  std::string cmdLine = "npm install -g @deepseek-ai/dsh";
  if (preferLocalTgz) {
    std::string m;
    if (globMatch(packagesDir() + "\\dsh-*.tgz", m)) cmdLine = "npm install -g \"" + m + "\"";
  }
  CmdResult r = runCmd(cmdLine, 900000);
  if (r.ran && r.code == 0) println(a.tr(LK_INST_OK), CLR_GREEN);
  else println(a.f(LK_INST_FAIL, r.code), CLR_RED);
  showCmdTail(r, 6);
  verifyPrint(a, "Deepseek Harness", chkDsh);
}

static void installDshNpx(App& a) {
  if (!haveNode(a)) return;
  println(a.tr(LK_INST_RUN_INTER), CLR_YELLOW);
  runDetach(L"C:\\Windows\\System32\\cmd.exe", L"/k npx --yes @deepseek-ai/dsh");
}

static void installDshSource(App& a) {
  if (!haveNode(a)) return;
  Chk g = chkGit();
  if (!g.found) { println(a.tr(LK_INST_GIT_MISSING), CLR_RED); return; }
  std::string ps = appDir() + "\\install-dsh-src.ps1";
  std::string psrc;
  psrc += "\r$ErrorActionPreference='Stop'\r\r\n";
  psrc += "\r$dir=Join-Path $env:LOCALAPPDATA 'TC-tools\\dsh-src'\r\r\n";
  psrc += "\rif(!(Test-Path $dir)){ git clone --depth 1 https://github.com/deepseek-ai/deepseek-harness.git $dir | Out-Host }\r\r\n";
  psrc += "\rPush-Location $dir\r\r\n";
  psrc += "\rgit pull --ff-only 2>$null\r\r\n";
  psrc += "\rcorepack enable 2>$null\r\r\n";
  psrc += "\rcorepack prepare pnpm@11.7.0 --activate 2>$null\r\r\n";
  psrc += "\rpnpm install | Out-Host\r\r\n";
  psrc += "\rpnpm run build:official | Out-Host\r\r\n";
  psrc += "\rPush-Location apps\\cli\r\r\n";
  psrc += "\rpnpm link --global | Out-Host\r\r\n";
  psrc += "\rPop-Location\r\r\n";
  psrc += "\rPop-Location\r\r\n";
  psrc += "\rWrite-Output 'DSH_SOURCE_DONE'\r\r\n";
  writeTextFile(ps, psrc);
  println(a.tr(LK_INST_RUN_SILENT), CLR_YELLOW);
  CmdResult r = runCmd("powershell.exe -NoProfile -ExecutionPolicy Bypass -File \"" + ps + "\"", 2700000);
  if (r.ran && contains(r.out, "DSH_SOURCE_DONE")) println(a.tr(LK_INST_OK), CLR_GREEN);
  else println(a.f(LK_INST_FAIL, r.code), CLR_RED);
  showCmdTail(r, 8);
  verifyPrint(a, "Deepseek Harness", chkDsh);
}
// tools.cpp - part3: bundles + office/ai + dispatch
#include "tools.hpp"
#include "http.hpp"
#include "json.hpp"

#include <windows.h>

#include <string>
#include <vector>

using namespace tcu;
using namespace tools;

// ---------------------------------------------------------------- bundles
static void installDevCpp(App& a, bool online) {
  std::string name = "Dev-Cpp 5.11 TDM-GCC 4.9.2 Setup.exe";
  std::string url = "https://sourceforge.net/projects/orwelldevcpp/files/Setup%20Releases/Dev-Cpp%205.11%20TDM-GCC%204.9.2%20Setup.exe/download";
  std::string file;
  if (online) file = downloadTo(a, url, name);
  else {
    file = packagesDir() + "\\" + name;
    if (!fileExists(file)) { println(a.f(LK_INST_LOCAL_MISSING, name.c_str()), CLR_RED); return; }
  }
  if (file.empty()) return;
  runSilent(a, u8w(file), L"/S");
  verifyPrint(a, "DEV-C++", chkDevCpp);
}

static void installVscode(App& a, bool online) {
  std::string name = "VSCodeUserSetup-x64-latest.exe";
  std::string url = "https://update.code.visualstudio.com/latest/win32-x64-user/stable";
  std::string file;
  if (online) file = downloadTo(a, url, name);
  else {
    file = packagesDir() + "\\" + name;
    if (!fileExists(file)) { println(a.f(LK_INST_LOCAL_MISSING, name.c_str()), CLR_RED); return; }
  }
  if (file.empty()) return;
  runSilent(a, u8w(file), L"/S");
  verifyPrint(a, "VS Code", chkVscode);
}

static void installPython(App& a, bool online) {
  std::string name = "python-3.13.15-amd64.exe";
  std::string url = "https://www.python.org/ftp/python/3.13.15/" + name;
  std::string file;
  if (online) file = downloadTo(a, url, name);
  else {
    file = packagesDir() + "\\" + name;
    if (!fileExists(file)) { println(a.f(LK_INST_LOCAL_MISSING, name.c_str()), CLR_RED); return; }
  }
  if (file.empty()) return;
  runSilent(a, u8w(file),
            L"/quiet InstallAllUsers=0 PrependPath=1 Include_launcher=1 Include_idle=1 Include_pip=1");
  verifyPrint(a, "Python", chkPython);
}

static void installIdeJetbrains(App& a, bool online, bool clion) {
  if (a.wv.build > 0 && a.wv.build < 17134) {
    println(a.tr(LK_INST_JB_NOTE), CLR_YELLOW);
  }
  std::string code = clion ? "CL" : "PCC";
  std::string name = clion ? "CLion-latest.exe" : "pycharm-community-latest.exe";
  std::string url = "https://data.services.jetbrains.com/products/download?code=" + code + "&platform=windows&type=release";
  std::string file;
  if (online) file = downloadTo(a, url, name);
  else {
    file = packagesDir() + "\\" + name;
    if (!fileExists(file)) { println(a.f(LK_INST_LOCAL_MISSING, name.c_str()), CLR_RED); return; }
  }
  if (file.empty()) return;
  runSilent(a, u8w(file), L"/S");
  if (clion) verifyPrint(a, "CLion", chkClion);
  else verifyPrint(a, "PyCharm", chkPyCharm);
}

static void installToolbox(App& a, bool online) {
  std::string name = "jetbrains-toolbox-latest.exe";
  std::string url = "https://data.services.jetbrains.com/products/download?code=TBA&platform=windows&type=release";
  std::string file;
  if (online) file = downloadTo(a, url, name);
  else {
    file = packagesDir() + "\\" + name;
    if (!fileExists(file)) { println(a.f(LK_INST_LOCAL_MISSING, name.c_str()), CLR_RED); return; }
  }
  if (file.empty()) return;
  runDetached(a, u8w(file), L"");
  verifyPrint(a, "JetBrains Toolbox", chkToolbox);
}

static void installQt(App& a, bool online) {
  std::string name = "qt-unified-windows-x64-online.exe";
  std::string url = "https://download.qt.io/official_releases/online_installers/" + name;
  std::string file;
  if (online) file = downloadTo(a, url, name);
  else {
    file = packagesDir() + "\\" + name;
    if (!fileExists(file)) { println(a.f(LK_INST_LOCAL_MISSING, name.c_str()), CLR_RED); return; }
  }
  if (file.empty()) return;
  println(a.tr(LK_INST_QT_NOTE), CLR_GRAY);
  runDetached(a, u8w(file), L"");
  verifyPrint(a, "Qt Creator", chkQtCreator);
}

struct OfficeSpec {
  const char* name;
  const char* localFile;
  const char* url;
};
static const OfficeSpec OFFICES[] = {
  { "WPS",      "wps.exe",      "https://www.wps.cn/" },
  { "QQ",       "qq.exe",       "https://im.qq.com/" },
  { "Seewo",    "seewo.exe",    "https://easinote.seewo.com/" },
  { "DingTalk", "dingtalk.exe", "https://www.dingtalk.com/" },
  { "WeChat",   "wechat.exe",   "https://pc.weixin.qq.com/" }
};

static void installOffice(App& a, bool online) {
  if (online) {
    println(a.f(LK_INST_START, a.tr(LK_IT_OFFICE).c_str()), CLR_GREEN);
    for (auto& o : OFFICES) {
      println(a.f(LK_INST_OFFLINE_PAGE, o.url), CLR_YELLOW);
      openUrl(o.url);
    }
    println(a.tr(LK_COMBO_DONE), CLR_GREEN);
    return;
  }
  std::string missing;
  for (auto& o : OFFICES) {
    std::string f = packagesDir() + "\\" + o.localFile;
    if (fileExists(f)) {
      println(a.f(LK_INST_START, o.name), CLR_GREEN);
      runDetached(a, u8w(f), L"");
    } else {
      if (!missing.empty()) missing += "; ";
      missing += o.localFile;
    }
  }
  if (!missing.empty()) println(a.f(LK_INST_LOCAL_MISSING, missing.c_str()), CLR_RED);
  println(a.tr(LK_COMBO_DONE), CLR_GREEN);
}

static void installAi(App& a) {
  if (is1709OrOlder()) {
    println(a.f(LK_INST_AI_BLOCK, a.wv.build), CLR_RED);
    kbWait();
    return;
  }
  if (!haveNode(a)) return;
  CmdResult r = runCmd("npm install -g @openai/codex @anthropic-ai/claude-code", 900000);
  if (r.ran && r.code == 0) {
    println(a.tr(LK_INST_OK), CLR_GREEN);
    println("- codex: 运行 codex 登录后续聊；codex 需要 Node 20+。", CLR_GRAY);
    println("- claude: 运行 claude 登录；Windows 上建议配合 Git for Windows。", CLR_GRAY);
  } else {
    println(a.f(LK_INST_FAIL, r.code), CLR_RED);
    showCmdTail(r, 8);
  }
  Chk c1 = chkCodex(), c2 = chkClaude();
  if (c1.found) println(a.f(LK_INST_CONFIRM_AI, ("codex " + c1.ver).c_str()), CLR_GREEN);
  if (c2.found) println(a.f(LK_INST_CONFIRM_AI, ("claude code " + c2.ver).c_str()), CLR_GREEN);
}

// ---------------------------------------------------------------- dispatch
std::string tools::verOf(int item) {
  switch (item) {
    case I_NODE:    return "v22.23.2 (LTS, 适配1709)";
    case I_MINGW:   return "13.2.0 (UCRT, r8)";
    case I_GIT:     return "2.55.0.5";
    case I_MSYS2:   return "2026-06-11";
    case I_PNPM:    return "10.x";
    case I_DSH:     return "0.1.2-rc.1+";
    case I_DSH_NPM: return "npm -g 最新";
    case I_DSH_NPX: return "npx 最新";
    case I_DSH_GIT: return "源码 master";
    case I_C1:      return "DEV-C++ 5.11.0 + VS Code";
    case I_C2:      return "PyCharm Community + Python 3.13.15 + Toolbox";
    case I_C3:      return "CLion + MinGW-w64 13.2 + Toolbox";
    case I_C4:      return "Qt online installer (建议 5.15.x)";
    case I_OFFICE:  return "WPS/QQ/希沃/钉钉/微信";
    case I_AI:      return "codex + claude code (需 1803+)";
    default:        return "";
  }
}

int tools::labelOf(int item) {
  switch (item) {
    case I_NODE:    return LK_IT_NODE;
    case I_MINGW:   return LK_IT_MINGW;
    case I_GIT:     return LK_IT_GIT;
    case I_MSYS2:   return LK_IT_MSYS2;
    case I_PNPM:    return LK_IT_PNPM;
    case I_DSH:     return LK_IT_DSH;
    case I_DSH_NPM: return LK_IT_DSH_NPM;
    case I_DSH_NPX: return LK_IT_DSH_NPX;
    case I_DSH_GIT: return LK_IT_DSH_GIT;
    case I_C1:      return LK_IT_C1;
    case I_C2:      return LK_IT_C2;
    case I_C3:      return LK_IT_C3;
    case I_C4:      return LK_IT_C4;
    case I_OFFICE:  return LK_IT_OFFICE;
    case I_AI:      return LK_IT_AI;
    default:        return LK_BACK;
  }
}

void installBundle1(App& a, bool online);
void installBundle2(App& a, bool online);
void installBundle3(App& a, bool online);
void installBundle4(App& a, bool online);

static void installSub(App& a, int item, bool online) {
  // pre-check: OS + already installed (except bundles which check per part)
  if (!osGuard(a)) return;
  switch (item) {
    case I_NODE: {
      Chk c = chkNode();
      if (c.found) { println(a.f(LK_INST_ALREADY, a.tr(LK_IT_NODE).c_str(), c.ver.c_str()), CLR_YELLOW); return; }
      installNode(a, online);
      return;
    }
    case I_MINGW: {
      Chk c = chkMingw();
      if (c.found) { println(a.f(LK_INST_ALREADY, a.tr(LK_IT_MINGW).c_str(), c.ver.c_str()), CLR_YELLOW); return; }
      installMingw(a, online);
      return;
    }
    case I_GIT: {
      Chk c = chkGit();
      if (c.found) { println(a.f(LK_INST_ALREADY, a.tr(LK_IT_GIT).c_str(), c.ver.c_str()), CLR_YELLOW); return; }
      installGit(a, online);
      return;
    }
    case I_MSYS2: {
      Chk c = chkMsys2();
      if (c.found) { println(a.f(LK_INST_ALREADY, a.tr(LK_IT_MSYS2).c_str(), c.ver.c_str()), CLR_YELLOW); return; }
      installMsys2(a, online);
      return;
    }
    case I_PNPM: {
      Chk c = chkPnpm();
      if (c.found) { println(a.f(LK_INST_ALREADY, a.tr(LK_IT_PNPM).c_str(), c.ver.c_str()), CLR_YELLOW); return; }
      installPnpm(a, online);
      return;
    }
    case I_DSH: {
      Chk c = chkDsh();
      if (c.found) { println(a.f(LK_INST_ALREADY, a.tr(LK_IT_DSH).c_str(), c.ver.c_str()), CLR_YELLOW); return; }
      installDshGlobal(a, true);
      return;
    }
    case I_DSH_NPM: {
      Chk c = chkDsh();
      if (c.found) { println(a.f(LK_INST_ALREADY, a.tr(LK_IT_DSH_NPM).c_str(), c.ver.c_str()), CLR_YELLOW); return; }
      installDshGlobal(a, false);
      return;
    }
    case I_DSH_NPX: {
      Chk c = chkDsh();
      if (c.found) { println(a.f(LK_INST_ALREADY, a.tr(LK_IT_DSH_NPX).c_str(), c.ver.c_str()), CLR_YELLOW); return; }
      installDshNpx(a);
      return;
    }
    case I_DSH_GIT: {
      Chk c = chkDsh();
      if (c.found) { println(a.f(LK_INST_ALREADY, a.tr(LK_IT_DSH_GIT).c_str(), c.ver.c_str()), CLR_YELLOW); return; }
      installDshSource(a);
      return;
    }
    case I_C1: installBundle1(a, online); return;
    case I_C2: installBundle2(a, online); return;
    case I_C3: installBundle3(a, online); return;
    case I_C4: installBundle4(a, online); return;
    case I_OFFICE: installOffice(a, online); return;
    case I_AI: installAi(a); return;
    default: return;
  }
}

void tools::installItem(App& a, int item, bool online) {
  println(a.f(LK_INST_START, a.tr(labelOf(item)).c_str()), CLR_GREEN);
  installSub(a, item, online);
}
// tools.cpp - part4: bundle implementations
#include "tools.hpp"
#include <string>

using namespace tcu;
using namespace tools;

static const char* BUNDLE_BAR = "===== %s =====";

void installBundle1(App& a, bool online) {
  if (!buildAtLeast(15063)) { println(a.f(LK_INST_OS_BLOCK, a.wv.name.c_str()), CLR_RED); return; }
  println(sf(BUNDLE_BAR, a.tr(LK_IT_C1).c_str()), CLR_CYAN);
  {
    Chk c = chkDevCpp();
    if (c.found) println(a.f(LK_INST_ALREADY, "Bloodshed DEV-C++ 5.11.0", c.ver.c_str()), CLR_YELLOW);
    else { installDevCpp(a, online); }
  }
  {
    Chk c = chkVscode();
    if (c.found) println(a.f(LK_INST_ALREADY, "VS Code", c.ver.c_str()), CLR_YELLOW);
    else { installVscode(a, online); }
  }
  println(a.tr(LK_COMBO_DONE), CLR_GREEN);
}

void installBundle2(App& a, bool online) {
  println(sf(BUNDLE_BAR, a.tr(LK_IT_C2).c_str()), CLR_CYAN);
  {
    Chk c = chkPython();
    if (c.found) println(a.f(LK_INST_ALREADY, "Python 3 (含 IDLE)", c.ver.c_str()), CLR_YELLOW);
    else installPython(a, online);
  }
  {
    Chk c = chkPyCharm();
    if (c.found) println(a.f(LK_INST_ALREADY, "PyCharm", c.ver.c_str()), CLR_YELLOW);
    else installIdeJetbrains(a, online, false);
  }
  {
    Chk c = chkToolbox();
    if (c.found) println(a.f(LK_INST_ALREADY, "JetBrains Toolbox", c.ver.c_str()), CLR_YELLOW);
    else installToolbox(a, online);
  }
  println(a.tr(LK_COMBO_DONE), CLR_GREEN);
}

void installBundle3(App& a, bool online) {
  println(sf(BUNDLE_BAR, a.tr(LK_IT_C3).c_str()), CLR_CYAN);
  {
    Chk c = chkMingw();
    if (c.found) println(a.f(LK_INST_ALREADY, "MinGW-w64 13.2", c.ver.c_str()), CLR_YELLOW);
    else installMingw(a, online);
  }
  {
    Chk c = chkClion();
    if (c.found) println(a.f(LK_INST_ALREADY, "CLion", c.ver.c_str()), CLR_YELLOW);
    else installIdeJetbrains(a, online, true);
  }
  {
    Chk c = chkToolbox();
    if (c.found) println(a.f(LK_INST_ALREADY, "JetBrains Toolbox", c.ver.c_str()), CLR_YELLOW);
    else installToolbox(a, online);
  }
  println(a.tr(LK_COMBO_DONE), CLR_GREEN);
}

void installBundle4(App& a, bool online) {
  println(sf(BUNDLE_BAR, a.tr(LK_IT_C4).c_str()), CLR_CYAN);
  {
    Chk c = chkQtCreator();
    if (c.found) println(a.f(LK_INST_ALREADY, "Qt Creator", c.ver.c_str()), CLR_YELLOW);
    else installQt(a, online);
  }
  println(a.tr(LK_COMBO_DONE), CLR_GREEN);
}
// tools.cpp - part5: check page implementation
#include "tools.hpp"
#include <string>
#include <vector>

using namespace tcu;
using namespace tools;

static void printFound(App& a, const std::string& name, const Chk& c) {
  if (c.found) {
    println(a.f(LK_CHK_VER, c.ver.c_str()), CLR_GREEN);
    if (!c.where.empty()) println(a.f(LK_CHK_PATH, c.where.c_str()), CLR_GRAY);
  } else {
    println(a.f(LK_CHK_NOTFOUND, name.c_str()), CLR_RED);
  }
}

void tools::runCheckItem(App& a, int item) {
  println(" " + std::string(60, '='), CLR_CYAN);
  switch (item) {
    case LK_C2: {
      println(a.f(LK_CHK_HEADER, a.tr(LK_C2).c_str()), CLR_CYAN);
      Chk c = chkNode();
      printFound(a, "Node.js", c);
      if (c.found) {
        auto n = runCmd("npm --version", 20000);
        if (n.ran && n.code == 0) println(a.f(LK_CHK_VER, ("npm " + trim(n.out)).c_str()), CLR_GREEN);
        else println(a.tr(LK_CHK_ERROR) + " npm", CLR_RED);
        auto t = runCmd("node -e \"console.log('node-runtime-ok')\"", 20000);
        if (t.ran && t.code == 0) println(a.tr(LK_CHK_OK) + " (node -e)", CLR_GREEN);
        else println(a.tr(LK_CHK_ERROR) + " node -e", CLR_RED);
      } else println(a.tr(LK_TIP_NODE), CLR_YELLOW);
      break;
    }
    case LK_C3: {
      println(a.f(LK_CHK_HEADER, a.tr(LK_C3).c_str()), CLR_CYAN);
      Chk c = chkGit();
      printFound(a, "Git", c);
      if (c.found) {
        auto t = runCmd("git config --global --get user.name", 15000);
        if (!t.ran || t.code != 0 || trim(t.out).empty())
          println("  [i] git 尚未配置 user.name（建议：git config --global user.name \"your name\"）", CLR_YELLOW);
      } else println(a.tr(LK_TIP_GIT), CLR_YELLOW);
      break;
    }
    case LK_C4: {
      println(a.f(LK_CHK_HEADER, a.tr(LK_C4).c_str()), CLR_CYAN);
      Chk c = chkPnpm();
      printFound(a, "pnpm", c);
      if (!c.found) println(a.tr(LK_TIP_PNPM), CLR_YELLOW);
      break;
    }
    case LK_C5: {
      println(a.f(LK_CHK_HEADER, a.tr(LK_C5).c_str()), CLR_CYAN);
      Chk c = chkMsys2();
      printFound(a, "MSYS2", c);
      if (c.found) {
        std::string b = c.where.find("bash") != std::string::npos ? c.where : "C:\\msys64\\usr\\bin\\bash.exe";
        if (fileExists(b)) {
          auto t = runCmd("\"" + b + "\" -lc \"echo msys-ok\"", 25000);
          if (t.ran && t.code == 0) println(a.tr(LK_CHK_OK), CLR_GREEN);
          else println(a.tr(LK_CHK_ERROR) + " bash", CLR_RED);
        }
      } else println(a.tr(LK_TIP_MSYS2), CLR_YELLOW);
      break;
    }
    case LK_C6: {
      println(a.f(LK_CHK_HEADER, a.tr(LK_C6).c_str()), CLR_CYAN);
      Chk n = chkNode();
      printFound(a, "Node.js (dsh 依赖)", n);
      Chk c = chkDsh();
      printFound(a, "Deepseek Harness (dsh)", c);
      if (!c.found) println(a.tr(LK_TIP_DSH), CLR_YELLOW);
      break;
    }
    case LK_C7: {
      println(a.f(LK_CHK_HEADER, a.tr(LK_C7).c_str()), CLR_CYAN);
      Chk c = chkClaude();
      printFound(a, "Claude Code", c);
      if (!c.found) println(a.tr(LK_TIP_CLAUDE), CLR_YELLOW);
      break;
    }
    case LK_C8: {
      println(a.f(LK_CHK_HEADER, a.tr(LK_C8).c_str()), CLR_CYAN);
      Chk c = chkCodex();
      printFound(a, "Codex", c);
      if (!c.found) println(a.tr(LK_TIP_CODEX), CLR_YELLOW);
      break;
    }
    case LK_C9: {
      println(a.f(LK_CHK_HEADER, a.tr(LK_C9).c_str()), CLR_CYAN);
      Chk c = chkEdge();
      printFound(a, "Microsoft Edge", c);
      if (!c.found) println(a.tr(LK_TIP_EDGE), CLR_YELLOW);
      break;
    }
    case LK_C10: {
      println(a.f(LK_CHK_HEADER, a.tr(LK_C10).c_str()), CLR_CYAN);
      Chk c = chkChrome();
      printFound(a, "Google Chrome", c);
      if (!c.found) println(a.tr(LK_TIP_CHROME), CLR_YELLOW);
      break;
    }
    case LK_C11: {
      println(a.f(LK_CHK_HEADER, a.tr(LK_C11).c_str()), CLR_CYAN);
      Chk c = chkDevCpp();
      printFound(a, "Bloodshed DEV-C++ 5.11.0", c);
      if (!c.found) println(a.tr(LK_TIP_DEVCPP), CLR_YELLOW);
      break;
    }
    case LK_C12: {
      println(a.f(LK_CHK_HEADER, a.tr(LK_C12).c_str()), CLR_CYAN);
      Chk c = chkMingw();
      printFound(a, "MinGW-w64 (gcc)", c);
      if (c.found) {
        std::string dir = c.where.substr(0, c.where.find_last_of("\\/") + 1);
        if (fileExists(dir + "g++.exe")) println(a.f(LK_CHK_SHELLOK, "g++"), CLR_GRAY);
        else println("  [i] 未找到 g++.exe（C++ 编译器）", CLR_YELLOW);
      } else println(a.tr(LK_TIP_MINGW), CLR_YELLOW);
      break;
    }
    case LK_C13: {
      println(a.f(LK_CHK_HEADER, a.tr(LK_C13).c_str()), CLR_CYAN);
      Chk c = chkPython();
      printFound(a, "Python 3", c);
      if (c.found) {
        auto id = runCmd("python -c \"import idlelib; print('idle-ok')\"", 20000);
        if (id.ran && id.code == 0) println("  [OK] IDLE 可用", CLR_GREEN);
        else println("  [X] IDLE 不可用（可选包），可重新安装并勾选 tcl/tk 与 IDLE", CLR_YELLOW);
      } else println(a.tr(LK_TIP_PYTHON), CLR_YELLOW);
      break;
    }
    case LK_C14: {
      println(a.f(LK_CHK_HEADER, a.tr(LK_C14).c_str()), CLR_CYAN);
      std::pair<std::string, Chk(*)(void)> list[] = {
        { "VS Code", chkVscode }, { "PyCharm", chkPyCharm }, { "CLion", chkClion },
        { "JetBrains Toolbox", chkToolbox }, { "Qt Creator", chkQtCreator }, { "DEV-C++", chkDevCpp }
      };
      int found = 0;
      for (auto& p : list) {
        Chk c = p.second();
        if (c.found) { ++found; printFound(a, p.first, c); }
        else println(a.f(LK_CHK_NOTFOUND, p.first.c_str()), CLR_RED);
      }
      println(a.f(LK_CHK_SUMMARY_TOTAL, found) + " / " + sf("%d", (int)(sizeof(list) / sizeof(list[0]))), CLR_GRAY);
      if (found == 0) println(a.tr(LK_TIP_IDE), CLR_YELLOW);
      break;
    }
    case LK_C15: {
      println(a.f(LK_CHK_HEADER, a.tr(LK_C15).c_str()), CLR_CYAN);
      auto list = chkOffice();
      int found = 0;
      for (auto& o : list) {
        if (o.found) { ++found; println(a.f(LK_CHK_VER, (o.name + " " + o.ver).c_str()), CLR_GREEN); }
        else println(a.f(LK_CHK_NOTFOUND, o.name.c_str()), CLR_RED);
      }
      if (found == 0) println(a.tr(LK_TIP_OFFICE), CLR_YELLOW);
      break;
    }
  }
  kbWait();
}

void tools::runCheckSummary(App& a) {
  println(" " + std::string(60, '='), CLR_CYAN);
  println(a.tr(LK_CHK_SUMMARY_HEAD), CLR_CYAN);
  std::pair<std::string, Chk(*)(void)> list[] = {
    { "Node.js", chkNode }, { "Git", chkGit }, { "pnpm", chkPnpm }, { "MSYS2", chkMsys2 },
    { "Deepseek Harness", chkDsh }, { "Claude Code", chkClaude }, { "Codex", chkCodex },
    { "Microsoft Edge", chkEdge }, { "Google Chrome", chkChrome }, { "DEV-C++", chkDevCpp },
    { "MinGW-w64", chkMingw }, { "Python", chkPython }, { "VS Code", chkVscode },
    { "PyCharm", chkPyCharm }, { "CLion", chkClion }, { "JetBrains Toolbox", chkToolbox },
    { "Qt Creator", chkQtCreator }
  };
  int found = 0;
  for (auto& p : list) {
    Chk c = p.second();
    if (c.found) {
      ++found;
      println("  [OK] " + p.first + "  " + c.ver, CLR_GREEN);
    } else {
      println("  [X ] " + p.first, CLR_RED);
    }
  }
  auto off = chkOffice();
  for (auto& o : off) {
    if (o.found) { ++found; println("  [OK] " + o.name + "  " + o.ver, CLR_GREEN); }
    else println("  [X ] " + o.name, CLR_RED);
  }
  println(a.f(LK_CHK_SUMMARY_TOTAL, found), CLR_GRAY);
  kbWait();
}