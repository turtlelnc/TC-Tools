// app.hpp - application state
#pragma once
#include "lang.hpp"
#include "util.hpp"
#include <string>

struct App {
  bool en = false;               // false = 中文 (default)
  tcu::WinVer wv;

  std::string tr(int id) const;                 // language string
  std::string f(int id, ...) const;             // language string printf-style
  void loadConfig();
  void saveConfig() const;
};

// page entry points (pages.cpp)
int pageHome(App& a);        // returns -1 exit, 1..3 = sub pages
void pageInstallMenu(App& a);
void pageChecks(App& a);
void pageCli(App& a);
void pageWebsite(App& a);

// pages.h helpers
int readChoiceNum(int minC, int maxC, App& a);
void itemLine(App& a, int no, int label, const char* extra0 = nullptr, const char* extra1 = nullptr);
