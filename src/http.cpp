// http.cpp - WinHTTP-based HTTP client (works on Win10 1709)
#include "http.hpp"
#include "util.hpp"

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#include <windows.h>
#include <winhttp.h>

#include <cstdio>
#include <string>
#include <vector>

namespace tch {

static std::string g_lastErr;

std::string lastName() { return g_lastErr; }

static std::string errText(DWORD e) {
  switch (e) {
    case ERROR_WINHTTP_CANNOT_CONNECT:    return "cannot connect (12029)";
    case ERROR_WINHTTP_TIMEOUT:           return "timeout (12002)";
    case ERROR_WINHTTP_NAME_NOT_RESOLVED: return "DNS resolution failed (12007)";
    case ERROR_WINHTTP_SECURE_FAILURE:    return "TLS/secure connection failed (12175)";
    case ERROR_WINHTTP_CONNECTION_ERROR:  return "connection lost (12030)";
    case ERROR_WINHTTP_UNRECOGNIZED_SCHEME: return "unsupported scheme (111)";
    case ERROR_WINHTTP_INVALID_SERVER_RESPONSE: return "invalid server response (12152)";
    case ERROR_WINHTTP_RESEND_REQUEST:    return "request needs resend (12032)";
    default:
      char* msg = nullptr;
      FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                     nullptr, e, 0, (char*)&msg, 0, nullptr);
      std::string r = msg ? msg : tcu::sf("WinHTTP error %lu", (unsigned long)e);
      if (msg) LocalFree(msg);
      return tcu::trim(r);
  }
}

static HINTERNET g_session = nullptr;
static HINTERNET sess() {
  if (!g_session) {
    g_session = WinHttpOpen(L"TC-tools/0.1.0-rc2", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (g_session) {
      const DWORD proto = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1 |
                          WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | 0x20 /* TLS1.3 on newer OS */;
      WinHttpSetOption(g_session, WINHTTP_OPTION_SECURE_PROTOCOLS, (LPVOID)&proto, sizeof(proto));
      WinHttpSetTimeouts(g_session, 30000, 60000, 30000, 300000);
    }
  }
  return g_session;
}

// ---- URL -> host/port/path ----
struct UrlParts { std::wstring host; INTERNET_PORT port = 0; std::wstring path; bool https = false; };
static bool splitUrl(const std::string& url, UrlParts& out, std::string* err) {
  std::wstring uw = tcu::u8w(url);
  URL_COMPONENTS uc{};
  uc.dwStructSize = sizeof(uc);
  wchar_t host[512] = {}, path[8192] = {}, user[128] = {}, pass[128] = {}, extra[2048] = {};
  uc.lpszHostName = host; uc.dwHostNameLength = 512;
  uc.lpszUrlPath = path; uc.dwUrlPathLength = 8192;
  uc.lpszExtraInfo = extra; uc.dwExtraInfoLength = 2048;
  uc.lpszUserName = user; uc.dwUserNameLength = 128;
  uc.lpszPassword = pass; uc.dwPasswordLength = 128;
  if (!WinHttpCrackUrl(uw.c_str(), (DWORD)uw.size(), 0, &uc)) {
    if (err) *err = tcu::sf("invalid URL: %s", url.c_str());
    return false;
  }
  out.host = std::wstring(host, uc.dwHostNameLength);
  std::wstring p = std::wstring(uc.lpszUrlPath, uc.dwUrlPathLength);
  if (uc.dwExtraInfoLength) p += std::wstring(extra, uc.dwExtraInfoLength);
  if (p.empty()) p = L"/";
  out.path = p;
  out.port = uc.nPort;
  out.https = (uc.nScheme == INTERNET_SCHEME_HTTPS);
  return true;
}

static std::wstring joinHeaders(const std::vector<std::string>& h) {
  std::wstring out;
  for (auto& x : h) { out += tcu::u8w(x); out += L"\r\n"; }
  return out;
}

static bool isRedirect(long st) {
  return st == 301 || st == 302 || st == 303 || st == 307 || st == 308;
}
static long doRequest(const std::string& url, const std::wstring& method,
                      const std::vector<std::string>& headers, const std::string& body,
                      const std::function<bool(const char*, size_t)>& sink,
                      HttpResp* resp, std::string* err, std::wstring* locationOut) {
  g_lastErr.clear();
  if (!sess()) { if (err) *err = "WinHttpOpen failed"; return 0; }
  UrlParts up;
  if (!splitUrl(url, up, err)) return 0;

  HINTERNET hConn = WinHttpConnect(sess(), up.host.c_str(), up.port, 0);
  if (!hConn) { if (err) *err = errText(GetLastError()); return 0; }
  DWORD flags = up.https ? WINHTTP_FLAG_SECURE : 0;
  HINTERNET hReq = WinHttpOpenRequest(hConn, method.c_str(), up.path.c_str(), nullptr,
                                      WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
  if (!hReq) { if (err) *err = errText(GetLastError()); WinHttpCloseHandle(hConn); return 0; }

  std::wstring hdr = joinHeaders(headers);
  if (!hdr.empty())
    WinHttpAddRequestHeaders(hReq, hdr.c_str(), (DWORD)hdr.size(), WINHTTP_ADDREQ_FLAG_ADD);

  BOOL ok = WinHttpSendRequest(hReq, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                               body.empty() ? WINHTTP_NO_REQUEST_DATA : (LPVOID)body.data(),
                               (DWORD)body.size(), (DWORD)body.size(), 0);
  if (!ok) { if (err) *err = errText(GetLastError()); WinHttpCloseHandle(hReq); WinHttpCloseHandle(hConn); return 0; }
  if (!WinHttpReceiveResponse(hReq, nullptr)) {
    if (err) *err = errText(GetLastError());
    WinHttpCloseHandle(hReq); WinHttpCloseHandle(hConn);
    return 0;
  }

  long status = 0;
  {
    DWORD sc = 0, len = sizeof(sc);
    if (WinHttpQueryHeaders(hReq, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            WINHTTP_HEADER_NAME_BY_INDEX, &sc, &len, WINHTTP_NO_HEADER_INDEX))
      status = (long)sc;
  }
  std::wstring ct;
  {
    wchar_t buf[256] = {}; DWORD len = sizeof(buf);
    if (WinHttpQueryHeaders(hReq, WINHTTP_QUERY_CONTENT_TYPE, WINHTTP_HEADER_NAME_BY_INDEX,
                            buf, &len, WINHTTP_NO_HEADER_INDEX))
      ct = buf;
  }
  if (resp) { resp->status = status; resp->contentType = tcu::wu8(ct); }

  if (locationOut) {
    wchar_t buf[2048] = {}; DWORD len = sizeof(buf);
    if (WinHttpQueryHeaders(hReq, WINHTTP_QUERY_LOCATION, WINHTTP_HEADER_NAME_BY_INDEX,
                            buf, &len, WINHTTP_NO_HEADER_INDEX))
      *locationOut = buf;
  }

  const size_t cap = 8u * 1024 * 1024;
  for (;;) {
    DWORD avail = 0;
    if (!WinHttpQueryDataAvailable(hReq, &avail)) {
      DWORD e = GetLastError();
      if (e == ERROR_WINHTTP_CONNECTION_ERROR || e == ERROR_WINHTTP_OPERATION_CANCELLED) break;
      if (err && err->empty()) *err = errText(e);
      break;
    }
    if (avail == 0) break;
    if (avail > 256 * 1024) avail = 256 * 1024;
    std::string buf(avail, 0);
    DWORD got = 0;
    if (!WinHttpReadData(hReq, (LPVOID)buf.data(), avail, &got)) break;
    if (got == 0) break;
    if (resp && resp->body.size() < cap) {
      size_t room = cap - resp->body.size();
      resp->body.append(buf.data(), got < room ? got : room);
    }
    if (sink && !sink(buf.data(), got)) break;
  }

  WinHttpCloseHandle(hReq);
  WinHttpCloseHandle(hConn);
  return status;
}

bool getText(const std::string& url, HttpResp& resp, const std::vector<std::string>& headers, std::string* err) {
  resp = HttpResp{};
  std::string cur = url;
  for (int hop = 0; hop < 8; ++hop) {
    std::wstring loc;
    long st = doRequest(cur, L"GET", headers, "", nullptr, &resp, err, &loc);
    if (st == 0) return false;
    if (isRedirect(st) && !loc.empty()) {
      cur = tcu::wu8(loc);
      continue;
    }
    return true;
  }
  if (err) *err = "too many redirects";
  return false;
}

bool download(const std::string& url, const std::string& filePath,
              const std::vector<std::string>& headers,
              const std::function<void(long long got, long long total)>& progress,
              std::string* err) {
  std::string cur = url;
  for (int hop = 0; hop < 8; ++hop) {
    long st = doRequest(cur, L"GET", headers, "", nullptr, nullptr, err, nullptr);
    if (st == 0) return false;
    if (isRedirect(st)) {
      std::wstring l2;
      long st2 = doRequest(cur, L"GET", headers, "", nullptr, nullptr, err, &l2);
      (void)st2;
      if (!l2.empty()) { cur = tcu::wu8(l2); continue; }
    }
    break;
  }
  DWORD retries = 3;
restart:
  HANDLE hFile = CreateFileW(tcu::u8w(filePath).c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL, nullptr);
  if (hFile == INVALID_HANDLE_VALUE) { if (err) *err = "cannot create output file"; return false; }
  long long got = 0;
  auto sink = [&](const char* d, size_t n) -> bool {
    DWORD w = 0;
    BOOL ok = WriteFile(hFile, d, (DWORD)n, &w, nullptr);
    if (ok) got += (long long)w;
    return ok;
  };
  long st = doRequest(cur, L"GET", headers, "", sink, nullptr, err, nullptr);
  CloseHandle(hFile);
  if (st == 0) return false;
  if (isRedirect(st)) {
    std::wstring l2; long st2 = doRequest(cur, L"GET", headers, "", nullptr, nullptr, err, &l2); (void)st2;
    if (!l2.empty()) { cur = tcu::wu8(l2); if (--retries) goto restart; }
    if (err) *err = "redirect chain too long";
    return false;
  }
  if (progress) progress(got, 0);
  return true;
}

bool postStream(const std::string& url, const std::vector<std::string>& headers,
                const std::string& body,
                const std::function<bool(const std::string& chunk)>& onChunk,
                std::string* err, HttpResp* resp) {
  auto sink = [&](const char* d, size_t n) -> bool {
    if (!onChunk) return true;
    return onChunk(std::string(d, n));
  };
  long st = doRequest(url, L"POST", headers, body, sink, resp, err, nullptr);
  if (st == 0) return false;
  return true;
}

} // namespace tch