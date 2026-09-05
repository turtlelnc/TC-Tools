// util.hpp - IO / process / registry / system helpers
// TC-tools v0.1.0-rc1
#pragma once
#include <string>
#include <vector>

namespace tcu {

// ---- console ----
void init();
bool isConsole();
enum Clr : int { CLR_DEF = 7, CLR_GRAY = 8, CLR_BLUE = 9, CLR_GREEN = 10, CLR_CYAN = 11, CLR_RED = 12, CLR_MAGENTA = 13, CLR_YELLOW = 14, CLR_WHITE = 15 };
void print(const std::string& s, int c = CLR_DEF);
void println(const std::string& s, int c = CLR_DEF);
void resume();
std::string readLine();
std::string readLineMasked();

// ---- strings ----
std::string sf(const char* fmt, ...);
std::string trim(const std::string& s);
std::string lowerA(const std::string& s);
bool startsWith(const std::string& s, const std::string& pre);
bool contains(const std::string& hay, const std::string& needle);

// ---- utf8 / wide ----
std::wstring u8w(const std::string& s);
std::string wu8(const std::wstring& s);

// ---- files ----
std::string exeDir();                       // dir of running exe
std::string appDir();                       // %APPDATA%\TC-tools (created)
std::string packagesDir();                  // <exeDir>\packages
bool fileExists(const std::string& p);
bool writeTextFile(const std::string& p, const std::string& t);
std::string readTextFile(const std::string& p);

// ---- process ----
struct CmdResult { bool ran = false; int code = -1; std::string out; };
CmdResult runCmd(const std::string& cmdLine, unsigned timeoutMs = 25000);
// 0 = ok, 1 = failed, 2 = canceled/UAC denied
int runInstallExe(const std::wstring& exe, const std::wstring& args, bool quiet);
void runDetach(const std::wstring& exe, const std::wstring& args);

// ---- system ----
struct WinVer { int major = 0, minor = 0, build = 0; std::string name; };
WinVer getWinVer();
bool buildAtLeast(int b);
bool is1709OrOlder();
std::string whereCmd(const std::string& name);

// ---- registry / versions ----
struct FoundApp { bool found = false; std::string name; std::string version; };
FoundApp findUninstall(const std::string& needle);   // case-insensitive DisplayName contains
std::string fileVersionStr(const std::string& path);
bool userPathHas(const std::string& item);
void userPathAdd(const std::string& item);

// ---- misc ----
bool openUrl(const std::string& url);
void kbWait();
std::string readConfig(const std::string& key, const std::string& dflt);
void writeConfig(const std::string& key, const std::string& val);

} // namespace tcu