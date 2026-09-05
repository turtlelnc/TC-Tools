// app.cpp
#include "app.hpp"
#include <cstdarg>
#include <cstdio>
#include <string>

const LangEntry* langTable();

std::string App::tr(int id) const {
  const LangEntry* L = langTable();
  if (id < 0 || id >= LK_COUNT) return "";
  return en ? L[id].en : L[id].zh;
}

std::string App::f(int id, ...) const {
  std::string fmt = tr(id);
  va_list ap; va_start(ap, id);
  char buf[4096];
  vsnprintf(buf, sizeof(buf), fmt.c_str(), ap);
  va_end(ap);
  return std::string(buf);
}

void App::loadConfig() {
  std::string v = tcu::readConfig("language", "zh");
  en = (v == "en");
  wv = tcu::getWinVer();
}

void App::saveConfig() const {
  tcu::writeConfig("language", en ? "en" : "zh");
}
