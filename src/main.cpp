// main.cpp - TC-tools entry
#include "app.hpp"

#include <windows.h>

int main() {
  tcu::init();
  SetConsoleTitleW(L"TC-tools v0.1.0-rc1");

  App a;
  a.loadConfig();

  if (!tcu::buildAtLeast(15063)) {
    tcu::println(a.f(LK_OS_UNSUPPORTED, a.wv.name.c_str()), tcu::CLR_RED);
    tcu::kbWait();
    return 1;
  }

  for (;;) {
    int r = pageHome(a);
    if (r == -1) break;
    if (r == 1) pageInstallMenu(a);
    else if (r == 2) pageChecks(a);
    else pageCli(a);
  }
  return 0;
}
