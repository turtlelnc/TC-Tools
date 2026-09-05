// http.hpp - WinHTTP wrapper (GET/POST/stream, Win10 1709 compatible)
#pragma once
#include <functional>
#include <string>
#include <vector>

struct HttpResp {
  long status = 0;          // 0 = transport error
  std::string body;
  std::string contentType;
};

namespace tch {
// GET whole response into resp.body (cap ~8 MB)
bool getText(const std::string& url, HttpResp& resp,
             const std::vector<std::string>& headers = {}, std::string* err = nullptr);
// GET, write to file; progress(got,total)
bool download(const std::string& url, const std::string& filePath,
              const std::vector<std::string>& headers,
              const std::function<void(long long got, long long total)>& progress,
              std::string* err = nullptr);
// POST JSON (or any body) with streaming callback; onChunk returns false to abort
bool postStream(const std::string& url, const std::vector<std::string>& headers,
                const std::string& body,
                const std::function<bool(const std::string& chunk)>& onChunk,
                std::string* err = nullptr, HttpResp* resp = nullptr);
// last transport error text (for friendly messages)
std::string lastName();
} // namespace tch
