// tools.hpp - toolchain install & check
#pragma once
#include "app.hpp"
#include <string>
#include <vector>

namespace tools {

enum Item {
  I_NODE, I_MINGW, I_GIT, I_MSYS2, I_PNPM,
  I_DSH, I_DSH_NPM, I_DSH_NPX, I_DSH_GIT,
  I_C1, I_C2, I_C3, I_C4, I_OFFICE, I_AI, I_BACK
};

struct Chk { bool found = false; std::string ver; std::string where; };
struct OfficeChk { std::string name; bool found = false; std::string ver; };

// --- checks ---
Chk chkNode();
Chk chkGit();
Chk chkPnpm();
Chk chkMsys2();
Chk chkDsh();
Chk chkClaude();
Chk chkCodex();
Chk chkEdge();
Chk chkChrome();
Chk chkDevCpp();
Chk chkMingw();
Chk chkPython();
Chk chkVscode();
Chk chkPyCharm();
Chk chkClion();
Chk chkToolbox();
Chk chkQtCreator();
std::vector<OfficeChk> chkOffice();

// --- install ---
void installItem(App& a, int item, bool online);

// version label for menus
std::string verOf(int item);
// item display label key
int labelOf(int item);

// used by pages: run a generic check item (prints result+tips)
void runCheckItem(App& a, int item);
void runCheckSummary(App& a);

} // namespace tools
