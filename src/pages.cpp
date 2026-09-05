// pages.cpp - menu pages
#include "app.hpp"
#include "tools.hpp"
#include "cli.hpp"

#include <cstdlib>
#include <string>

using namespace tcu;

static int readChoiceNum(App& a, int minC, int maxC) {
  for (;;) {
    print(a.tr(LK_HINT_CHOOSE), CLR_YELLOW);
    std::string s = trim(readLine());
    if (s.empty()) continue;
    int n = atoi(s.c_str());
    if (n >= minC && n <= maxC) return n;
    println(a.tr(LK_INVALID_CHOICE), CLR_RED);
  }
}

static void banner(App& a) {
  println("  ____  _____  ____   ____            ", CLR_CYAN);
  println(" |_   _|_   _|  _ \\ / ___|  _ __  ___  ", CLR_CYAN);
  println("   | |   | | | | | | |    | '_ \\/ __| ", CLR_CYAN);
  println("   | |   | | | |_| | |___ | | | \\__ \\ ", CLR_CYAN);
  println("   |_|   |_| |____/ \\____||_| |_|___/  ", CLR_CYAN);
  println("  " + a.f(LK_VERSION, "v0.1.0-rc1") + "  |  Windows 10 1709+", CLR_WHITE);
  println("  " + a.f(LK_OS_LINE, a.wv.name.c_str()), CLR_GRAY);
  println("  " + a.f(LK_LANG_LINE, (a.en ? "English" : "简体中文")), CLR_GRAY);
}

int pageHome(App& a) {
  for (;;) {
    banner(a);
    println("", CLR_DEF);
    println(a.tr(LK_HOME_INTRO), CLR_YELLOW);
    println("", CLR_DEF);
    println(sf("  %d. %s", 1, a.tr(LK_M1).c_str()), CLR_DEF);
    println(sf("  %d. %s", 2, a.tr(LK_M2).c_str()), CLR_DEF);
    println(sf("  %d. %s", 3, a.tr(LK_M3).c_str()), CLR_DEF);
    println(sf("  %d. %s", 4, a.tr(LK_M4).c_str()), CLR_DEF);
    println(sf("  %d. %s", 5, a.tr(LK_M5).c_str()), CLR_DEF);
    println(sf("  %d. %s", 6, a.tr(LK_M6).c_str()), CLR_DEF);
    println("", CLR_DEF);
    int n = readChoiceNum(a, 1, 6);
    switch (n) {
      case 1: return 1;
      case 2: return 2;
      case 3: return 3;
      case 4:
        a.en = !a.en;
        a.saveConfig();
        println(a.f(LK_LANG_SWITCHED, (a.en ? "English" : "简体中文")), CLR_GREEN);
        break;
      case 5:
        openUrl("https://turtleweb.cc.cd");
        println(a.tr(LK_WEB_OPENED), CLR_GREEN);
        kbWait();
        break;
      case 6:
        println(a.tr(LK_GOODBYE), CLR_GREEN);
        return -1;
    }
  }
}

static void printItemLine(App& a, const char* no, int item, bool online) {
  std::string extra = a.f(LK_IT_VER, tools::verOf(item).c_str());
  println(sf("  %s. %s%s", no, a.tr(tools::labelOf(item)).c_str(), extra.c_str()), CLR_DEF);
}

static int localItems[] = { tools::I_NODE, tools::I_MINGW, tools::I_GIT, tools::I_MSYS2, tools::I_PNPM,
                            tools::I_DSH, tools::I_C1, tools::I_C2, tools::I_C3, tools::I_C4,
                            tools::I_OFFICE, tools::I_AI };

static void pageLocalInstall(App& a) {
  for (;;) {
    println(" " + std::string(60, '='), CLR_CYAN);
    println("  " + a.tr(LK_P1_TITLE), CLR_CYAN);
    println("  " + a.tr(LK_P1_LOCAL), CLR_WHITE);
    println("  " + a.tr(LK_P1_COMMENT_LOCAL), CLR_GRAY);
    println("", CLR_DEF);
    for (int i = 0; i < (int)(sizeof(localItems) / sizeof(int)); ++i)
      printItemLine(a, sf("%d", i + 1).c_str(), localItems[i], false);
    println(sf("  %d. %s", 13, a.tr(LK_BACK).c_str()), CLR_DEF);
    println("", CLR_DEF);
    int n = readChoiceNum(a, 1, 13);
    if (n == 13) break;
    tools::installItem(a, localItems[n - 1], false);
    kbWait();
  }
}

static void pageOnlineInstall(App& a) {
  for (;;) {
    println(" " + std::string(60, '='), CLR_CYAN);
    println("  " + a.tr(LK_P1_TITLE), CLR_CYAN);
    println("  " + a.tr(LK_P1_ONLINE), CLR_WHITE);
    println("  " + a.tr(LK_P1_COMMENT_ONLINE), CLR_GRAY);
    println("", CLR_DEF);
    printItemLine(a, "1", tools::I_NODE, true);
    printItemLine(a, "2", tools::I_MINGW, true);
    printItemLine(a, "3", tools::I_GIT, true);
    printItemLine(a, "4", tools::I_MSYS2, true);
    printItemLine(a, "5", tools::I_PNPM, true);
    printItemLine(a, "5A", tools::I_DSH_NPM, true);
    printItemLine(a, "5B", tools::I_DSH_NPX, true);
    printItemLine(a, "5C", tools::I_DSH_GIT, true);
    printItemLine(a, "6", tools::I_C1, true);
    printItemLine(a, "7", tools::I_C2, true);
    printItemLine(a, "8", tools::I_C3, true);
    printItemLine(a, "9", tools::I_C4, true);
    printItemLine(a, "10", tools::I_OFFICE, true);
    printItemLine(a, "11", tools::I_AI, true);
    println(sf("  %d. %s", 12, a.tr(LK_BACK).c_str()), CLR_DEF);
    println("", CLR_DEF);
    print(a.tr(LK_HINT_CHOOSE), CLR_YELLOW);
    std::string s = trim(readLine());
    if (s.empty()) { println(a.tr(LK_INVALID_CHOICE), CLR_RED); continue; }
    std::string up = lowerA(s);
    int item = -1;
    if (up == "5a") item = tools::I_DSH_NPM;
    else if (up == "5b") item = tools::I_DSH_NPX;
    else if (up == "5c") item = tools::I_DSH_GIT;
    else {
      int n = atoi(s.c_str());
      switch (n) {
        case 1: item = tools::I_NODE; break;
        case 2: item = tools::I_MINGW; break;
        case 3: item = tools::I_GIT; break;
        case 4: item = tools::I_MSYS2; break;
        case 5: item = tools::I_PNPM; break;
        case 6: item = tools::I_C1; break;
        case 7: item = tools::I_C2; break;
        case 8: item = tools::I_C3; break;
        case 9: item = tools::I_C4; break;
        case 10: item = tools::I_OFFICE; break;
        case 11: item = tools::I_AI; break;
        case 12: item = -2; break; // back
        default: item = -1;
      }
    }
    if (item == -2) break;
    if (item < 0) { println(a.tr(LK_INVALID_CHOICE), CLR_RED); continue; }
    tools::installItem(a, item, true);
    kbWait();
  }
}

void pageInstallMenu(App& a) {
  for (;;) {
    println(" " + std::string(60, '='), CLR_CYAN);
    println("  " + a.tr(LK_P1_TITLE), CLR_CYAN);
    println("", CLR_DEF);
    println(sf("  %d. %s", 1, a.tr(LK_P1_LOCAL).c_str()), CLR_DEF);
    println("     " + a.tr(LK_P1_COMMENT_LOCAL), CLR_GRAY);
    println(sf("  %d. %s", 2, a.tr(LK_P1_ONLINE).c_str()), CLR_DEF);
    println("     " + a.tr(LK_P1_COMMENT_ONLINE), CLR_GRAY);
    println(sf("  %d. %s", 3, a.tr(LK_BACK).c_str()), CLR_DEF);
    println("", CLR_DEF);
    int n = readChoiceNum(a, 1, 3);
    if (n == 1) pageLocalInstall(a);
    else if (n == 2) pageOnlineInstall(a);
    else break;
  }
}

void pageChecks(App& a) {
  for (;;) {
    println(" " + std::string(60, '='), CLR_CYAN);
    println("  " + a.tr(LK_P2_TITLE), CLR_CYAN);
    println("", CLR_DEF);
    const int keys[] = { LK_C1, LK_C2, LK_C3, LK_C4, LK_C5, LK_C6, LK_C7, LK_C8,
                         LK_C9, LK_C10, LK_C11, LK_C12, LK_C13, LK_C14, LK_C15, LK_C16 };
    for (int i = 0; i < 16; ++i)
      println(sf("  %d. %s", i + 1, a.tr(keys[i]).c_str()), i == 15 ? CLR_DEF : CLR_DEF);
    println("", CLR_DEF);
    int n = readChoiceNum(a, 1, 16);
    if (n == 1) tools::runCheckSummary(a);
    else if (n == 16) break;
    else tools::runCheckItem(a, keys[n - 1]);
  }
}

void pageCli(App& a) {
  for (;;) {
    println(" " + std::string(60, '='), CLR_CYAN);
    println("  " + a.tr(LK_P3_TITLE), CLR_CYAN);
    println("", CLR_DEF);
    println(a.tr(LK_P3_DESC1), CLR_GRAY);
    println(a.tr(LK_P3_DESC2), CLR_GRAY);
    println(a.tr(LK_P3_DESC3), CLR_GRAY);
    println("", CLR_DEF);
    println(sf("  %d. %s", 1, a.tr(LK_P3_START).c_str()), CLR_WHITE);
    println(sf("  %d. %s", 2, a.tr(LK_BACK).c_str()), CLR_DEF);
    println("", CLR_DEF);
    int n = readChoiceNum(a, 1, 2);
    if (n == 2) break;
    cliMain(a);
  }
}