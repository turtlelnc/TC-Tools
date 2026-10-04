// bootctl.cpp - 开机启动 / SYSTEM 解锁服务（控制台 UI）
// TC-tools v0.2.0-rc3
#include "bootctl.hpp"
#include "unlock.hpp"      // unlock::pageChoice（与解锁页共用菜单输入）
#include "util.hpp"

#include <cstdlib>
#include <string>
#include <vector>
#include <windows.h>
#include <shellapi.h>

using namespace tcu;

namespace bootctl {

// 从单行 JSON 里取字段（容错：不同版组件字段名可能略有差异）
static std::string jsonStr(const std::string& j, const std::string& key) {
  const std::string k = "\"" + key + "\"";
  size_t p = j.find(k);
  if (p == std::string::npos) return "";
  p = j.find(':', p + k.size());
  if (p == std::string::npos) return "";
  ++p;
  while (p < j.size() && (j[p] == ' ' || j[p] == '\t')) ++p;
  if (p < j.size() && j[p] == '"') {
    ++p;
    std::string out;
    while (p < j.size() && j[p] != '"') { if (j[p] == '\\' && p + 1 < j.size()) ++p; out += j[p++]; }
    return out;
  }
  size_t e = p;
  while (e < j.size() && j[e] != ',' && j[e] != '}') ++e;
  return trim(j.substr(p, e - p));
}

static bool jsonBoolAny(const std::string& j, std::initializer_list<const char*> keys) {
  for (const char* k : keys) {
    std::string v = jsonStr(j, k);
    if (v == "true") return true;
    if (v == "false") return false;
  }
  return false;
}

bool supported(const std::string& helper) {
  CmdResult r = runCmd("\"" + helper + "\" help", 20000);
  return r.ran && contains(r.out, "boot");
}

BootState query(const std::string& helper) {
  BootState s;
  if (helper.empty()) return s;
  CmdResult r = runCmd("\"" + helper + "\" boot status --json", 25000);
  if (!r.ran || r.out.empty()) return s;
  s.ok = true;
  s.installed  = jsonBoolAny(r.out, { "installed", "taskInstalled", "systemTaskInstalled", "enabled" });
  s.elevated   = jsonBoolAny(r.out, { "elevated", "isElevated" });
  s.isSystem   = jsonBoolAny(r.out, { "isSystem" });
  s.migration  = jsonBoolAny(r.out, { "secretMigrated", "machineSecretsReady", "migrated" });
  s.taskName   = jsonStr(r.out, "taskName");
  s.detail     = jsonStr(r.out, "detail");
  return s;
}

// 以管理员身份运行组件子命令（触发 UAC）。返回退出码，-1 = 未启动/被拒绝。
static int runElevated(const std::string& helper, const std::string& args) {
  std::wstring file = u8w(helper);
  std::wstring params = u8w(args);
  SHELLEXECUTEINFOW sei{};
  sei.cbSize = sizeof(sei);
  sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
  sei.lpVerb = L"runas";
  sei.lpFile = file.c_str();
  sei.lpParameters = params.c_str();
  sei.nShow = SW_SHOWNORMAL;
  if (!ShellExecuteExW(&sei)) {
    DWORD e = GetLastError();
    return (e == ERROR_CANCELLED) ? -1 : -2;
  }
  WaitForSingleObject(sei.hProcess, 120000);
  DWORD ec = 0;
  GetExitCodeProcess(sei.hProcess, &ec);
  CloseHandle(sei.hProcess);
  return (int)ec;
}

static void showState(App& a, const BootState& s) {
  if (!s.ok) { println("  " + a.tr(LK_BT_UNKNOWN), CLR_YELLOW); return; }
  println(a.tr(s.installed ? LK_BT_ON : LK_BT_OFF), s.installed ? CLR_GREEN : CLR_YELLOW);
  println(a.tr(s.migration ? LK_BT_KEY_OK : LK_BT_KEY_MISSING), s.migration ? CLR_GREEN : CLR_YELLOW);
  if (!s.taskName.empty()) println("  " + a.f(LK_BT_TASK, s.taskName.c_str()), CLR_GRAY);
  if (!s.elevated) println("  " + a.tr(LK_BT_NOT_ADMIN), CLR_GRAY);
}

void pageBoot(App& a, const std::string& helper) {
  if (helper.empty()) return;
  if (!supported(helper)) {
    println("  " + a.tr(LK_BT_UNSUPPORTED), CLR_RED);
    println("  " + a.tr(LK_BT_UNSUPPORTED_HINT), CLR_GRAY);
    kbWait();
    return;
  }

  for (;;) {
    BootState s = query(helper);
    println(" " + std::string(60, '='), CLR_CYAN);
    println("  " + a.tr(LK_BT_TITLE), CLR_CYAN);
    println("", CLR_DEF);
    println("  " + a.tr(LK_BT_WHY1), CLR_GRAY);
    println("  " + a.tr(LK_BT_WHY2), CLR_GRAY);
    println("", CLR_DEF);
    showState(a, s);
    println("", CLR_DEF);
    println(sf("  %d. %s", 1, a.tr(LK_BT_INSTALL).c_str()), CLR_DEF);
    println(sf("  %d. %s", 2, a.tr(LK_BT_UNINSTALL).c_str()), CLR_DEF);
    println(sf("  %d. %s", 3, a.tr(LK_BT_REFRESH).c_str()), CLR_DEF);
    println(sf("  %d. %s", 4, a.tr(LK_BT_BACK).c_str()), CLR_DEF);
    println("", CLR_DEF);
    int n = unlock::pageChoice(1, 4, a);
    if (n == 4) break;          // 返回上一页
    if (n == 3) continue;       // 刷新状态

    if (n == 1) {
      // ---- 安装开机启动（SYSTEM）----
      println("", CLR_DEF);
      println("  " + a.tr(LK_BT_WARN_TITLE), CLR_YELLOW);
      println("  " + a.tr(LK_BT_WARN1), CLR_YELLOW);
      println("  " + a.tr(LK_BT_WARN2), CLR_YELLOW);
      println("  " + a.tr(LK_BT_WARN3), CLR_GRAY);
      println("", CLR_DEF);
      print("  " + a.tr(LK_BT_CONFIRM), CLR_YELLOW);
      std::string yn = trim(readLine());
      if (!(yn == "y" || yn == "Y")) { println(a.tr(LK_CANCELED), CLR_GRAY); continue; }
      println("  " + a.tr(LK_BT_UAC_NOTE), CLR_GRAY);
      int ec = runElevated(helper, "boot install");
      if (ec == -1) println(a.tr(LK_BT_UAC_DENIED), CLR_RED);
      else if (ec == 0) println(a.tr(LK_BT_INSTALL_OK), CLR_GREEN);
      else println(a.f(LK_BT_FAILED, ec), CLR_RED);
      kbWait();
    } else if (n == 2) {
      // ---- 卸载开机启动 ----
      print("  " + a.tr(LK_BT_CONFIRM_OFF), CLR_YELLOW);
      std::string yn = trim(readLine());
      if (!(yn == "y" || yn == "Y")) { println(a.tr(LK_CANCELED), CLR_GRAY); continue; }
      int ec = runElevated(helper, "boot uninstall");
      if (ec == -1) println(a.tr(LK_BT_UAC_DENIED), CLR_RED);
      else if (ec == 0) println(a.tr(LK_BT_UNINSTALL_OK), CLR_GREEN);
      else println(a.f(LK_BT_FAILED, ec), CLR_RED);
      kbWait();
    }
  }
}

} // namespace bootctl
