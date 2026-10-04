// bootctl.hpp - 开机启动 / SYSTEM 解锁服务（C++ UI 层）
// TC-tools v0.2.0-rc3
//
// 真实工作由解锁组件 tctool-unlock.exe 的 `boot` 子命令完成：
//   boot status|install|uninstall [--json] [--run-elevated]
// 本层只负责：状态展示、以管理员方式触发安装、以及把「为什么需要管理员」讲清楚。
#pragma once
#include "app.hpp"
#include <string>

namespace bootctl {

// 组件的 boot 子命令是否可用（旧版组件没有该命令）
bool supported(const std::string& helper);

// 解析 boot status --json 的最小结果
struct BootState {
  bool ok = false;            // 是否成功取到状态
  bool installed = false;     // 系统级（SYSTEM）开机任务是否存在且指向本程序
  bool elevated = false;      // 当前进程是否有管理员权限
  bool isSystem = false;      // 当前进程是否是 SYSTEM
  bool migration = false;     // 机器级密钥（供 SYSTEM 读取）是否已就绪
  std::string taskName;
  std::string detail;
};

BootState query(const std::string& helper);

// 子页面（解锁页第 7 项）
void pageBoot(App& a, const std::string& helper);

} // namespace bootctl
