// json.hpp - minimal JSON parser/serializer (UTF-8)
#pragma once
#include <map>
#include <string>
#include <vector>

struct Json {
  enum T { NUL, BOOL, NUM, STR, ARR, OBJ };
  T t = NUL;
  bool b = false;
  double n = 0;
  std::string s;
  std::vector<Json> arr;
  std::map<std::string, Json> obj;

  bool isObj() const { return t == OBJ; }
  const Json* get(const std::string& k) const {
    auto it = obj.find(k);
    return it == obj.end() ? nullptr : &it->second;
  }
  std::string str(const std::string& k, const std::string& d = "") const {
    const Json* j = get(k);
    return (!j || j->t != STR) ? d : j->s;
  }
  static bool parse(const std::string& text, Json& out);
  static std::string esc(const std::string& s);
};
