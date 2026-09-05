// util.cpp
#define WINVER 0x0A00
#define _WIN32_WINNT 0x0A00
#include "util.hpp"
#include "lang.hpp"

#include <windows.h>
#include <shellapi.h>

#include <cstdarg>
#include <cstdio>
#include <string>
#include <vector>

using namespace tcu;

static HANDLE g_out = INVALID_HANDLE_VALUE;
static HANDLE g_in = INVALID_HANDLE_VALUE;
static bool g_con = false;

void tcu::init() {
  g_out = GetStdHandle(STD_OUTPUT_HANDLE);
  g_in = GetStdHandle(STD_INPUT_HANDLE);
  DWORD m;
  g_con = (GetConsoleMode(g_out, &m) && GetConsoleMode(g_in, &m));
  if (g_con) {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    // ensure quick-edit does not freeze the app on click
    DWORD im; if (GetConsoleMode(g_in, &im)) SetConsoleMode(g_in, im | ENABLE_EXTENDED_FLAGS);
  }
}

bool tcu::isConsole() { return g_con; }

void tcu::print(const std::string& s, int col) {
  if (g_con) {
    SetConsoleTextAttribute(g_out, (WORD)col);
    std::wstring w = u8w(s);
    WriteConsoleW(g_out, w.c_str(), (DWORD)w.size(), nullptr, nullptr);
    SetConsoleTextAttribute(g_out, CLR_DEF);
  } else {
    DWORD n = 0;
    WriteFile(g_out, s.data(), (DWORD)s.size(), &n, nullptr);
  }
}

void tcu::println(const std::string& s, int col) { print(s, col); print("\r\n"); }

void tcu::resume() { print("\r\n"); }

std::string tcu::readLine() {
  std::string res;
  if (g_con) {
    wchar_t buf[4096]; DWORD got = 0;
    if (!ReadConsoleW(g_in, buf, 4095, &got, nullptr)) return "";
    res = wu8(std::wstring(buf, got));
  } else {
    char buf[4096];
    if (!std::fgets(buf, sizeof(buf), stdin)) return "";
    res = buf;
  }
  // strip UTF-8 BOM if present (e.g. piped input with BOM)
  if (res.size() >= 3 && (unsigned char)res[0] == 0xEF && (unsigned char)res[1] == 0xBB && (unsigned char)res[2] == 0xBF)
    res.erase(0, 3);
  while (!res.empty() && (res.back() == '\r' || res.back() == '\n')) res.pop_back();
  return res;
}

std::string tcu::readLineMasked() {
  if (!g_con) return readLine();
  DWORD old = 0; GetConsoleMode(g_in, &old);
  SetConsoleMode(g_in, old & ~DWORD(ENABLE_ECHO_INPUT));
  std::string s = readLine();
  SetConsoleMode(g_in, old);
  print("\r\n");
  return s;
}

std::string tcu::sf(const char* fmt, ...) {
  va_list ap; va_start(ap, fmt);
  char buf[2048];
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  return buf;
}

std::string tcu::trim(const std::string& s) {
  size_t a = s.find_first_not_of(" \t\r\n");
  if (a == std::string::npos) return "";
  size_t b = s.find_last_not_of(" \t\r\n");
  return s.substr(a, b - a + 1);
}

std::string tcu::lowerA(const std::string& s) {
  std::string r = s;
  for (auto& ch : r) if (ch >= 'A' && ch <= 'Z') ch += 32;
  return r;
}

bool tcu::startsWith(const std::string& s, const std::string& pre) {
  return s.size() >= pre.size() && s.compare(0, pre.size(), pre) == 0;
}

bool tcu::contains(const std::string& hay, const std::string& needle) {
  return hay.find(needle) != std::string::npos;
}

std::wstring tcu::u8w(const std::string& s) {
  if (s.empty()) return L"";
  int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
  std::wstring w; w.resize(n > 0 ? n : 1);
  if (n > 0) MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), &w[0], n);
  return w;
}
std::string tcu::wu8(const std::wstring& w) {
  if (w.empty()) return "";
  int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
  std::string s; s.resize(n > 0 ? n : 1);
  if (n > 0) WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), &s[0], n, nullptr, nullptr);
  return s;
}

static std::wstring wideDir(const std::wstring& p) {
  size_t k = p.find_last_of(L"\\/");
  return k == std::wstring::npos ? L"" : p.substr(0, k);
}

std::string tcu::exeDir() {
  wchar_t buf[MAX_PATH * 2]; DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH * 2);
  if (!n) return ".";
  return wu8(wideDir(std::wstring(buf, n)));
}

std::string tcu::appDir() {
  wchar_t buf[MAX_PATH * 2]; DWORD n = GetEnvironmentVariableW(L"APPDATA", buf, MAX_PATH * 2);
  std::string d;
  if (n > 0) d = wu8(std::wstring(buf, n)) + "\\TC-tools";
  else d = exeDir() + "\\TC-tools";
  CreateDirectoryW(u8w(d).c_str(), nullptr);
  return d;
}

std::string tcu::packagesDir() { return exeDir() + "\\packages"; }

bool tcu::fileExists(const std::string& p) {
  DWORD a = GetFileAttributesW(u8w(p).c_str());
  return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

bool tcu::writeTextFile(const std::string& p, const std::string& t) {
  HANDLE h = CreateFileW(u8w(p).c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (h == INVALID_HANDLE_VALUE) return false;
  DWORD w = 0;
  BOOL ok = WriteFile(h, t.data(), (DWORD)t.size(), &w, nullptr);
  CloseHandle(h);
  return ok && w == (DWORD)t.size();
}

std::string tcu::readTextFile(const std::string& p) {
  HANDLE h = CreateFileW(u8w(p).c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (h == INVALID_HANDLE_VALUE) return "";
  std::string out;
  char buf[8192]; DWORD r = 0;
  for (;;) {
    if (!ReadFile(h, buf, sizeof(buf), &r, nullptr) || r == 0) break;
    out.append(buf, r);
  }
  CloseHandle(h);
  return out;
}

// ---- subprocess with capture ----
static std::string decodeOut(const std::string& bytes) {
  // try utf-8; if invalid fall back to ANSI (GBK on zh-CN)
  int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, bytes.data(), (int)bytes.size(), nullptr, 0);
  if (n > 0) return bytes; // valid utf-8
  int m = MultiByteToWideChar(CP_ACP, 0, bytes.data(), (int)bytes.size(), nullptr, 0);
  if (m > 0) {
    std::wstring w; w.resize(m);
    MultiByteToWideChar(CP_ACP, 0, bytes.data(), (int)bytes.size(), &w[0], m);
    return wu8(w);
  }
  return bytes;
}

CmdResult tcu::runCmd(const std::string& cmdLine, unsigned timeoutMs) {
  CmdResult res;
  SECURITY_ATTRIBUTES sa{ sizeof(sa), nullptr, TRUE };
  HANDLE rd = nullptr, wr = nullptr;
  if (!CreatePipe(&rd, &wr, &sa, 0)) return res;
  SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);
  STARTUPINFOW si{}; si.cb = sizeof(si); si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdOutput = wr; si.hStdError = wr;
  PROCESS_INFORMATION pi{};
  std::wstring cmdw = u8w(cmdLine);
  std::vector<wchar_t> cmd(cmdw.begin(), cmdw.end()); cmd.push_back(0);
  BOOL ok = CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
  if (!ok) { CloseHandle(rd); CloseHandle(wr); return res; }
  CloseHandle(wr);
  res.ran = true;
  std::string out;
  DWORD start = GetTickCount();
  for (;;) {
    DWORD avail = 0;
    while (PeekNamedPipe(rd, nullptr, 0, nullptr, &avail, nullptr) && avail > 0) {
      char buf[4096]; DWORD got = 0;
      if (!ReadFile(rd, buf, avail > sizeof(buf) ? sizeof(buf) : avail, &got, nullptr) || got == 0) break;
      out.append(buf, got);
    }
    DWORD w = WaitForSingleObject(pi.hProcess, 30);
    bool done = (w == WAIT_OBJECT_0);
    if (done) break;
    if (GetTickCount() - start > timeoutMs) {
      TerminateProcess(pi.hProcess, 1);
      WaitForSingleObject(pi.hProcess, 1000);
      break;
    }
  }
  for (;;) {
    DWORD avail = 0;
    if (!PeekNamedPipe(rd, nullptr, 0, nullptr, &avail, nullptr) || avail == 0) break;
    char buf[4096]; DWORD got = 0;
    if (!ReadFile(rd, buf, avail > sizeof(buf) ? sizeof(buf) : avail, &got, nullptr) || got == 0) break;
    out.append(buf, got);
  }
  DWORD ec = 0; GetExitCodeProcess(pi.hProcess, &ec);
  res.code = (int)ec;
  res.out = decodeOut(trim(out));
  CloseHandle(rd); CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
  return res;
}

int tcu::runInstallExe(const std::wstring& exe, const std::wstring& args, bool quiet) {
  std::wstring cmdline = L"\"" + exe + L"\" " + args;
  std::vector<wchar_t> cmd(cmdline.begin(), cmdline.end()); cmd.push_back(0);
  STARTUPINFOW si{}; si.cb = sizeof(si);
  PROCESS_INFORMATION pi{};
  BOOL ok = CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);
  if (ok) {
    CloseHandle(pi.hThread);
    DWORD w = WaitForSingleObject(pi.hProcess, 1000 * 60 * 120);
    DWORD ec = 0; GetExitCodeProcess(pi.hProcess, &ec);
    CloseHandle(pi.hProcess);
    return ec == 0 ? 0 : 1;
  }
  DWORD err = GetLastError();
  if (err == ERROR_ELEVATION_REQUIRED) {
    SHELLEXECUTEINFOW sei{};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"runas";
    sei.lpFile = exe.c_str();
    sei.lpParameters = args.c_str();
    sei.nShow = quiet ? SW_HIDE : SW_SHOWNORMAL;
    if (ShellExecuteExW(&sei) && sei.hProcess) {
      WaitForSingleObject(sei.hProcess, 1000 * 60 * 120);
      DWORD ec = 0; GetExitCodeProcess(sei.hProcess, &ec);
      CloseHandle(sei.hProcess);
      return ec == 0 ? 0 : 1;
    }
    DWORD e2 = GetLastError();
    return (e2 == ERROR_CANCELLED) ? 2 : 1;
  }
  return 1;
}

void tcu::runDetach(const std::wstring& exe, const std::wstring& args) {
  std::wstring cmdline = L"\"" + exe + L"\" " + args;
  std::vector<wchar_t> cmd(cmdline.begin(), cmdline.end()); cmd.push_back(0);
  STARTUPINFOW si{}; si.cb = sizeof(si);
  PROCESS_INFORMATION pi{};
  CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE, nullptr, nullptr, &si, &pi);
  if (pi.hThread) CloseHandle(pi.hThread);
  if (pi.hProcess) CloseHandle(pi.hProcess);
}

WinVer tcu::getWinVer() {
  typedef LONG(WINAPI* RtlGetVersionFn)(PVOID);
  WinVer v;
  struct RtlOsVer {
    DWORD dwOSVersionInfoSize;
    DWORD dwMajorVersion; DWORD dwMinorVersion; DWORD dwBuildNumber; DWORD dwPlatformId;
    wchar_t szCSDVersion[128];
  } o{};
  o.dwOSVersionInfoSize = sizeof(o);
  auto fn = (RtlGetVersionFn)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion");
  if (fn) fn(&o);
  v.major = (int)o.dwMajorVersion; v.minor = (int)o.dwMinorVersion; v.build = (int)o.dwBuildNumber;
  switch (v.build) {
    case 15063: v.name = "Windows 10 1703"; break;
    case 16299: v.name = "Windows 10 1709"; break;
    case 17134: v.name = "Windows 10 1803"; break;
    case 17763: v.name = "Windows 10 1809"; break;
    case 18362: v.name = "Windows 10 1903"; break;
    case 18363: v.name = "Windows 10 1909"; break;
    case 19041: v.name = "Windows 10 2004"; break;
    case 19042: v.name = "Windows 10 20H2"; break;
    case 19043: v.name = "Windows 10 21H1"; break;
    case 19044: v.name = "Windows 10 21H2"; break;
    case 19045: v.name = "Windows 10 22H2"; break;
    default:
      if (v.build >= 22000) v.name = sf("Windows 11 (build %d)", v.build);
      else if (v.major >= 10) v.name = sf("Windows 10 (build %d)", v.build);
      else v.name = sf("Windows (build %d)", v.build);
  }
  return v;
}

bool tcu::buildAtLeast(int b) {
  WinVer v = getWinVer();
  return v.major > 10 || (v.major == 10 && v.build >= b);
}

bool tcu::is1709OrOlder() { return !buildAtLeast(17134); }

std::string tcu::whereCmd(const std::string& name) {
  auto r = runCmd("where " + name, 15000);
  if (!r.ran || r.code != 0) return "";
  return trim(r.out);
}

FoundApp tcu::findUninstall(const std::string& needle) {
  FoundApp res;
  std::wstring n = u8w(lowerA(needle));
  const wchar_t* roots[] = {
    L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
    L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"
  };
  for (int rootIdx = 0; rootIdx < 2 && !res.found; ++rootIdx) {
    for (int hive = 0; hive < 3 && !res.found; ++hive) {
      HKEY hk[] = { HKEY_LOCAL_MACHINE, HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER };
      HKEY key = nullptr;
      LONG lr = RegOpenKeyExW(hk[hive], roots[rootIdx], 0, KEY_READ, &key);
      if (lr != ERROR_SUCCESS) continue;
      wchar_t sub[512];
      for (DWORD i = 0; ; ++i) {
        DWORD len = 512;
        if (RegEnumKeyExW(key, i, sub, &len, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;
        wchar_t disp[1024] = {}, ver[128] = {};
        DWORD dl = sizeof(disp), vl = sizeof(ver);
        RegGetValueW(key, sub, L"DisplayName", RRF_RT_REG_SZ, nullptr, disp, &dl);
        RegGetValueW(key, sub, L"DisplayVersion", RRF_RT_REG_SZ, nullptr, ver, &vl);
        std::wstring dn = u8w(lowerA(wu8(disp)));
        if (dn.find(n) != std::wstring::npos) {
          res.found = true;
          res.name = wu8(disp);
          res.version = wu8(ver);
          break;
        }
      }
      RegCloseKey(key);
    }
  }
  return res;
}

std::string tcu::fileVersionStr(const std::string& path) {
  std::wstring wp = u8w(path);
  DWORD hnd = 0;
  DWORD sz = GetFileVersionInfoSizeW(wp.c_str(), &hnd);
  if (!sz) return "";
  std::vector<char> buf(sz);
  if (!GetFileVersionInfoW(wp.c_str(), 0, sz, buf.data())) return "";
  VS_FIXEDFILEINFO* fi = nullptr; UINT fsz = 0;
  if (!VerQueryValueW(buf.data(), L"\\", (void**)&fi, &fsz) || !fi) return "";
  return sf("%d.%d.%d.%d", HIWORD(fi->dwFileVersionMS), LOWORD(fi->dwFileVersionMS),
            HIWORD(fi->dwFileVersionLS), LOWORD(fi->dwFileVersionLS));
}

static bool userPathHasW(const std::wstring& item) {
  wchar_t buf[16384] = {};
  DWORD sz = sizeof(buf);
  LONG r = RegGetValueW(HKEY_CURRENT_USER, L"Environment", L"Path", RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ, nullptr, buf, &sz);
  if (r != ERROR_SUCCESS) return false;
  std::wstring cur = buf;
  std::wstring low = u8w(lowerA(wu8(item)));
  std::wstring curlow = u8w(lowerA(wu8(cur)));
  return curlow.find(low) != std::wstring::npos;
}

bool tcu::userPathHas(const std::string& item) { return userPathHasW(u8w(item)); }

void tcu::userPathAdd(const std::string& item) {
  if (userPathHas(item)) return;
  wchar_t buf[16384] = {};
  DWORD sz = sizeof(buf);
  LONG r = RegGetValueW(HKEY_CURRENT_USER, L"Environment", L"Path", RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ, nullptr, buf, &sz);
  std::wstring cur;
  if (r == ERROR_SUCCESS) cur = buf;
  if (!cur.empty() && cur.back() != L';') cur += L';';
  cur += u8w(item);
  HKEY k = nullptr;
  if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Environment", 0, nullptr, 0, KEY_SET_VALUE, nullptr, &k, nullptr) == ERROR_SUCCESS) {
    RegSetValueExW(k, L"Path", 0, REG_EXPAND_SZ, (const BYTE*)cur.c_str(), (DWORD)((cur.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(k);
  }
  SendMessageTimeoutW(HWND_BROADCAST, 0x1A, 0, (LPARAM)L"Environment", 0x2, 3000, nullptr);
}

bool tcu::openUrl(const std::string& url) {
  ShellExecuteW(nullptr, L"open", u8w(url).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
  return true;
}

void tcu::kbWait() { readLine(); }

static std::string cfgPath() { return appDir() + "\\config.ini"; }

std::string tcu::readConfig(const std::string& key, const std::string& dflt) {
  std::string t = readTextFile(cfgPath());
  std::string pat = key + "=";
  if (startsWith(t, pat)) return trim(t.substr(pat.size()));
  return dflt;
}

void tcu::writeConfig(const std::string& key, const std::string& val) {
  std::string t = readTextFile(cfgPath());
  std::string pat = key + "=";
  std::string rest;
  if (startsWith(t, pat)) rest = t.substr(pat.size());
  writeTextFile(cfgPath(), pat + val + "\r\n");
  (void)rest;
}