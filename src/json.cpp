// json.cpp
#include "json.hpp"
#include <cstdlib>
#include <cstring>

namespace {
struct P {
  const char* p; const char* e; bool ok = true;
  void skipWs() { while (p < e && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) ++p; }
  bool more() { return p < e; }
  char peek() { return p < e ? *p : 0; }

  void parseValue(Json& v) {
    if (!ok) return;
    skipWs();
    if (!more()) { ok = false; return; }
    char c = *p;
    if (c == '{') parseObj(v);
    else if (c == '[') parseArr(v);
    else if (c == '"') { v.t = Json::STR; v.s = parseString(); }
    else if (c == 't' && match("true")) { v.t = Json::BOOL; v.b = true; }
    else if (c == 'f' && match("false")) { v.t = Json::BOOL; v.b = false; }
    else if (c == 'n' && match("null")) { v.t = Json::NUL; }
    else if (c == '-' || (c >= '0' && c <= '9')) { v.t = Json::NUM; v.n = parseNum(); }
    else ok = false;
  }
  bool match(const char* w) {
    size_t n = strlen(w);
    if ((size_t)(e - p) < n || memcmp(p, w, n) != 0) return false;
    p += n; return true;
  }
  void parseObj(Json& v) {
    v.t = Json::OBJ;
    ++p; skipWs();
    if (peek() == '}') { ++p; return; }
    for (;;) {
      skipWs();
      if (peek() != '"') { ok = false; return; }
      std::string key = parseString();
      skipWs();
      if (peek() != ':') { ok = false; return; }
      ++p;
      Json val;
      parseValue(val);
      if (!ok) return;
      v.obj[key] = val;
      skipWs();
      if (peek() == ',') { ++p; continue; }
      if (peek() == '}') { ++p; return; }
      ok = false; return;
    }
  }
  void parseArr(Json& v) {
    v.t = Json::ARR;
    ++p; skipWs();
    if (peek() == ']') { ++p; return; }
    for (;;) {
      Json val;
      parseValue(val);
      if (!ok) return;
      v.arr.push_back(val);
      skipWs();
      if (peek() == ',') { ++p; continue; }
      if (peek() == ']') { ++p; return; }
      ok = false; return;
    }
  }
  std::string parseString() {
    std::string out;
    if (peek() == '"') ++p;
    while (more() && *p != '"') {
      unsigned char c = (unsigned char)*p;
      if (c == '\\') {
        ++p;
        if (!more()) { ok = false; return out; }
        char e = *p++;
        switch (e) {
          case '"': out += '"'; break;
          case '\\': out += '\\'; break;
          case '/': out += '/'; break;
          case 'b': out += '\b'; break;
          case 'f': out += '\f'; break;
          case 'n': out += '\n'; break;
          case 'r': out += '\r'; break;
          case 't': out += '\t'; break;
          case 'u': {
            unsigned u = hex4();
            if (u >= 0xD800 && u <= 0xDBFF && more() && p[0] == '\\' && p[1] == 'u') {
              p += 2; unsigned lo = hex4();
              if (lo >= 0xDC00 && lo <= 0xDFFF) u = 0x10000 + ((u - 0xD800) << 10) + (lo - 0xDC00);
            }
            appendUtf8(out, u);
            break;
          }
          default: out += e; break;
        }
      } else { out += (char)c; ++p; }
    }
    if (peek() == '"') ++p;
    return out;
  }
  unsigned hex4() {
    unsigned v = 0;
    for (int i = 0; i < 4; ++i) {
      if (!more()) { ok = false; return 0; }
      char c = *p++;
      v <<= 4;
      if (c >= '0' && c <= '9') v |= (unsigned)(c - '0');
      else if (c >= 'a' && c <= 'f') v |= (unsigned)(c - 'a' + 10);
      else if (c >= 'A' && c <= 'F') v |= (unsigned)(c - 'A' + 10);
      else { ok = false; return 0; }
    }
    return v;
  }
  static void appendUtf8(std::string& out, unsigned u) {
    if (u < 0x80) out += (char)u;
    else if (u < 0x800) { out += (char)(0xC0 | (u >> 6)); out += (char)(0x80 | (u & 0x3F)); }
    else if (u < 0x10000) { out += (char)(0xE0 | (u >> 12)); out += (char)(0x80 | ((u >> 6) & 0x3F)); out += (char)(0x80 | (u & 0x3F)); }
    else { out += (char)(0xF0 | (u >> 18)); out += (char)(0x80 | ((u >> 12) & 0x3F)); out += (char)(0x80 | ((u >> 6) & 0x3F)); out += (char)(0x80 | (u & 0x3F)); }
  }
  double parseNum() {
    const char* st = p;
    while (more() && ((*p >= '0' && *p <= '9') || *p == '-' || *p == '+' || *p == '.' || *p == 'e' || *p == 'E')) ++p;
    std::string tmp(st, p);
    return strtod(tmp.c_str(), nullptr);
  }
};
} // namespace

bool Json::parse(const std::string& text, Json& out) {
  P p{ text.data(), text.data() + text.size() };
  p.parseValue(out);
  p.skipWs();
  return p.ok && !p.more();
}

std::string Json::esc(const std::string& s) {
  std::string out;
  out.reserve(s.size() + 8);
  for (unsigned char c : s) {
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if (c < 0x20) { char b[8]; snprintf(b, sizeof(b), "\\u%04x", c); out += b; }
        else out += (char)c;
    }
  }
  return out;
}
