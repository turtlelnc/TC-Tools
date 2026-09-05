// cli.cpp - part1: vendors & config
#include "cli.hpp"
#include "json.hpp"
#include "http.hpp"

#include <windows.h>

#include <string>
#include <vector>

using namespace tcu;

struct Vendor {
  int id;
  int label;      // LK_CLI_V_*
  std::string baseUrl;
  std::string model;
  bool needKey;
};

static const Vendor VENDORS[] = {
  { 1, LK_CLI_V_DEEPSEEK, "https://api.deepseek.com",       "deepseek-chat", true },
  { 2, LK_CLI_V_OPENAI,   "https://api.openai.com/v1",       "gpt-4o-mini", true },
  { 3, LK_CLI_V_MOONSHOT, "https://api.moonshot.cn/v1",      "moonshot-v1-8k", true },
  { 4, LK_CLI_V_GLM,      "https://open.bigmodel.cn/api/paas/v4", "glm-4-flash", true },
  { 5, LK_CLI_V_QWEN,     "https://dashscope.aliyuncs.com/compatible-mode/v1", "qwen-plus", true },
  { 6, LK_CLI_V_SILICON,  "https://api.siliconflow.cn/v1",   "deepseek-ai/DeepSeek-V3", true },
  { 7, LK_CLI_V_ARK,      "https://ark.cn-beijing.volces.com/api/v3", "", true },
  { 8, LK_CLI_V_OLLAMA,   "http://localhost:11434/v1",       "llama3.2", false },
  { 9, LK_CLI_V_CUSTOM,   "",                                 "", true }
};

struct CliCfg {
  int vendor = -1;
  std::string baseUrl, model, key;
  bool saved = false;
};

static std::string cfgFile() { return appDir() + "\\cli-config.json"; }

static bool loadCfg(CliCfg& cfg) {
  std::string t = readTextFile(cfgFile());
  if (t.empty()) return false;
  Json j;
  if (!Json::parse(t, j) || !j.isObj()) return false;
  cfg.vendor = (int)(j.get("vendor") ? j.get("vendor")->n : -1);
  cfg.baseUrl = j.str("base_url");
  cfg.model = j.str("model");
  cfg.key = j.str("api_key");
  cfg.saved = true;
  return true;
}

static void saveCfg(const CliCfg& cfg) {
  std::string t = "{\"vendor\":" + std::to_string(cfg.vendor) +
                  ",\"base_url\":\"" + Json::esc(cfg.baseUrl) +
                  "\",\"model\":\"" + Json::esc(cfg.model) +
                  "\",\"api_key\":\"" + Json::esc(cfg.key) + "\"}";
  writeTextFile(cfgFile(), t);
}

static bool vendorChosen(const Vendor& v, CliCfg& cfg, App& a) {
  bool done = false;
  for (int attempt = 0; attempt < 2 && !done; ++attempt) {
    println(sf("  %s:", a.tr(v.label).c_str()), CLR_CYAN);
    if (v.id != 8) {
      print(a.tr(LK_CLI_KEY), CLR_YELLOW);
      cfg.key = trim(readLineMasked());
    }
    std::string url = v.baseUrl;
    std::string model = v.model;
    if (v.id == 9) {  // custom: ask everything
      print(a.tr(LK_CLI_URL), CLR_YELLOW); url = trim(readLine());
      print(a.tr(LK_CLI_MODEL), CLR_YELLOW); model = trim(readLine());
    } else if (v.id == 7) {  // ark: model needed
      print(a.tr(LK_CLI_MODEL), CLR_YELLOW); model = trim(readLine());
    }
    if (url.empty() || model.empty()) {
      println(a.tr(LK_INVALID_CHOICE), CLR_RED);
      continue;
    }
    cfg.baseUrl = url;
    cfg.model = model;
    done = true;
  }
  return done;
}
// cli.cpp - part2: chat main loop
#include "cli.hpp"
#include "json.hpp"
#include "http.hpp"

#include <string>
#include <vector>

using namespace tcu;

static std::string buildBody(const std::string& model, const std::vector<std::pair<std::string, std::string>>& msgs, bool stream) {
  std::string body = "{\"model\":\"" + Json::esc(model) + "\",\"stream\":" + (stream ? "true" : "false") + ",\"messages\":[";
  bool first = true;
  for (auto& m : msgs) {
    if (!first) body += ",";
    first = false;
    body += "{\"role\":\"" + Json::esc(m.first) + "\",\"content\":\"" + Json::esc(m.second) + "\"}";
  }
  body += "]}";
  return body;
}

// returns assistant text (may be empty on error); prints streamed deltas
static std::string chatOnce(App& a, const CliCfg& cfg, const std::vector<std::pair<std::string, std::string>>& msgs) {
  HttpResp resp;
  std::vector<std::string> headers = { "Content-Type: application/json" };
  if (!cfg.key.empty()) headers.push_back("Authorization: Bearer " + cfg.key);
  std::string err;
  std::string finalText;
  std::string buf;
  bool aborted = false;

  auto handleLine = [&](const std::string& line) -> bool {
    if (aborted) return false;
    if (line.find("data:") != 0) return true;
    std::string payload = trim(line.substr(5));
    if (payload.empty()) return true;
    if (payload == "[DONE]") { aborted = true; return false; }
    Json j;
    if (!Json::parse(payload, j)) return true;
    const Json* ch = j.get("choices");
    if (!ch || ch->t != Json::ARR || ch->arr.empty()) return true;
    const Json* delta = ch->arr[0].get("delta");
    const Json* msg = ch->arr[0].get("message");
    const Json* content = nullptr;
    if (delta) content = delta->get("content");
    if (!content && msg) content = msg->get("content");
    if (content && content->t == Json::STR && !content->s.empty()) {
      print(content->s, CLR_GREEN);
      finalText += content->s;
    }
    // reasoning printed as dim info
    const Json* rc = nullptr;
    if (delta) rc = delta->get("reasoning_content");
    if (rc && rc->t == Json::STR && !rc->s.empty()) {
      print(rc->s, CLR_GRAY);
      finalText += rc->s;
    }
    return true;
  };

  auto onChunk = [&](const std::string& chunk) -> bool {
    buf += chunk;
    for (;;) {
      size_t n = buf.find('\n');
      if (n == std::string::npos) break;
      std::string line = buf.substr(0, n);
      if (!line.empty() && line.back() == '\r') line.pop_back();
      buf.erase(0, n + 1);
      if (!line.empty()) if (!handleLine(line)) return false;
      if (aborted) return false;
    }
    return true;
  };

  bool ok = tch::postStream(cfg.baseUrl + "/chat/completions", headers,
                            buildBody(cfg.model, msgs, true), onChunk, &err, &resp);
  // flush remaining buffer
  if (ok && !aborted && !buf.empty()) {
    std::string line = trim(buf);
    if (line.find("data:") == 0) handleLine(line);
  }
  if (!ok) {
    println("\r\n" + a.f(LK_CLI_ERR_HTTP, err.c_str()), CLR_RED);
    return "";
  }
  if (resp.status != 200) {
    // try to extract error message
    std::string emsg;
    Json j;
    if (Json::parse(resp.body, j) && j.isObj()) {
      const Json* e = j.get("error");
      if (e) {
        if (e->t == Json::STR) emsg = e->s;
        else emsg = e->str("message");
      }
    }
    if (emsg.empty()) emsg = resp.body.substr(0, 200);
    println("\r\n" + a.f(LK_CLI_ERR_STATUS, (int)resp.status, emsg.c_str()), CLR_RED);
    return "";
  }
  // non-stream JSON fallback (some vendors ignore stream:true)
  if (finalText.empty() && !resp.body.empty() && aborted == false) {
    Json j;
    if (Json::parse(resp.body, j) && j.isObj()) {
      const Json* ch = j.get("choices");
      if (ch && ch->t == Json::ARR && !ch->arr.empty()) {
        const Json* msg = ch->arr[0].get("message");
        if (msg) {
          const Json* content = msg->get("content");
          if (content && content->t == Json::STR && !content->s.empty()) {
            print(content->s, CLR_GREEN);
            finalText = content->s;
          }
        }
      }
    }
  }
  return finalText;
}

void cliMain(App& a) {
  println(" " + std::string(60, '='), CLR_CYAN);
  println(a.f(LK_CHK_HEADER, a.tr(LK_P3_TITLE).c_str()), CLR_CYAN);

  CliCfg cfg;
  if (loadCfg(cfg)) {
    println(a.tr(LK_CLI_LOADED), CLR_GRAY);
  }
  if (cfg.baseUrl.empty()) {
    // pick vendor
    println(a.tr(LK_CLI_SEL_VENDOR), CLR_YELLOW);
    for (auto& v : VENDORS) {
      println(sf("  %d. %s", v.id, a.tr(v.label).c_str()), CLR_DEF);
    }
    int no = -1;
    for (;;) {
      print(a.tr(LK_HINT_CHOOSE), CLR_YELLOW);
      std::string s = trim(readLine());
      int n = atoi(s.c_str());
      if (n >= 1 && n <= 9) { no = n; break; }
      println(a.tr(LK_INVALID_CHOICE), CLR_RED);
    }
    const Vendor* v = &VENDORS[no - 1];
    cfg.vendor = v->id;
    if (!vendorChosen(*v, cfg, a)) { println(a.tr(LK_CANCELED), CLR_RED); return; }
    print(a.tr(LK_CLI_SAVE), CLR_YELLOW);
    std::string s = lowerA(trim(readLine()));
    if (s == "y" || s == "yes") {
      saveCfg(cfg);
      println(a.tr(LK_CLI_SAVED), CLR_GREEN);
      cfg.saved = true;
    }
  }
  if (cfg.baseUrl.empty() || cfg.model.empty()) { println(a.tr(LK_CLI_ERR_NOKEY), CLR_RED); return; }

  println(a.tr(LK_CLI_CHAT_NOTE), CLR_GRAY);
  println(a.tr(LK_CLI_HELP), CLR_GRAY);

  std::vector<std::pair<std::string, std::string>> msgs;
  for (;;) {
    print("\r\n" + a.tr(LK_CLI_PROMPT), CLR_CYAN);
    std::string line = trim(readLine());
    if (line == "/exit") break;
    if (line == "/help") { println(a.tr(LK_CLI_HELP), CLR_GRAY); continue; }
    if (line == "/clear") { msgs.clear(); println("  [cleared]", CLR_GRAY); continue; }
    if (line.empty()) continue;

    msgs.push_back({ "user", line });
    // cap history to last 24 messages
    while (msgs.size() > 24) msgs.erase(msgs.begin());

    std::string answer = chatOnce(a, cfg, msgs);
    print("\r\n", CLR_DEF);
    if (!answer.empty()) msgs.push_back({ "assistant", answer });
  }
  println(a.tr(LK_CLI_BYE), CLR_GREEN);
}