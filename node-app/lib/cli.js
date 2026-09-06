// cli.js - built-in lightweight CLI (LLM chat) - TC-tools Node.js edition
'use strict';
const path = require('path');
const core = require('./core');
const { print, println, readLine, readLineMasked, appDir, readText, writeText, postStream, firstLine } = core;

const VENDORS = [
  { id: 1, label: 'cliVDeeSeek', baseUrl: 'https://api.deepseek.com', model: 'deepseek-chat', needKey: true },
  { id: 2, label: 'cliVOpenAI', baseUrl: 'https://api.openai.com/v1', model: 'gpt-4o-mini', needKey: true },
  { id: 3, label: 'cliVMoonshot', baseUrl: 'https://api.moonshot.cn/v1', model: 'moonshot-v1-8k', needKey: true },
  { id: 4, label: 'cliVGLM', baseUrl: 'https://open.bigmodel.cn/api/paas/v4', model: 'glm-4-flash', needKey: true },
  { id: 5, label: 'cliVQwen', baseUrl: 'https://dashscope.aliyuncs.com/compatible-mode/v1', model: 'qwen-plus', needKey: true },
  { id: 6, label: 'cliVSilicon', baseUrl: 'https://api.siliconflow.cn/v1', model: 'deepseek-ai/DeepSeek-V3', needKey: true },
  { id: 7, label: 'cliVArk', baseUrl: 'https://ark.cn-beijing.volces.com/api/v3', model: '', needKey: true },
  { id: 8, label: 'cliVOllama', baseUrl: 'http://localhost:11434/v1', model: 'llama3.2', needKey: false },
  { id: 9, label: 'cliVCustom', baseUrl: '', model: '', needKey: true }
];

function esc(s) {
  return String(s).replace(/\\/g, '\\\\').replace(/"/g, '\\"').replace(/\r/g, '\\r').replace(/\n/g, '\\n').replace(/\t/g, '\\t');
}
function buildBody(model, msgs, stream) {
  let body = '{"model":"' + esc(model) + '","stream":' + (stream ? 'true' : 'false') + ',"messages":[';
  const parts = [];
  for (const m of msgs) parts.push('{"role":"' + esc(m[0]) + '","content":"' + esc(m[1]) + '"}');
  body += parts.join(',') + ']}';
  return body;
}

function cfgFile() { return path.join(appDir(), 'cli-config.json'); }
function loadCfg() {
  try {
    const t = readText(cfgFile());
    if (!t) return { vendor: -1, baseUrl: '', model: '', key: '' };
    const j = JSON.parse(t);
    return { vendor: j.vendor || -1, baseUrl: j.base_url || '', model: j.model || '', key: j.api_key || '' };
  } catch (e) { return { vendor: -1, baseUrl: '', model: '', key: '' }; }
}
function saveCfg(c) {
  writeText(cfgFile(), JSON.stringify({ vendor: c.vendor, base_url: c.baseUrl, model: c.model, api_key: c.key }));
}

async function vendorChosen(L, v, cfg) {
  try {
    println('  ' + L.t(v.label) + ':', 'cyan');
    if (v.id !== 8) {
      print(L.t('cliKey'), 'yellow');
      cfg.key = (await readLineMasked()).trim();
    }
    let url = v.baseUrl, model = v.model;
    if (v.id === 9) {
      print(L.t('cliUrl'), 'yellow'); url = (await readLine()).trim();
      print(L.t('cliModel'), 'yellow'); model = (await readLine()).trim();
    } else if (v.id === 7) {
      print(L.t('cliModel'), 'yellow'); model = (await readLine()).trim();
    }
    if (!url || !model) { println(L.t('invalidChoice'), 'red'); return false; }
    cfg.baseUrl = url; cfg.model = model;
    return true;
  } catch (e) { return false; }
}

async function chatOnce(L, cfg, msgs) {
  const headers = {};
  if (cfg.key) headers['Authorization'] = 'Bearer ' + cfg.key;
  let finalText = '';
  let buf = '';
  const handleLine = (line) => {
    if (line.indexOf('data:') !== 0) return true;
    const payload = line.slice(5).trim();
    if (!payload) return true;
    if (payload === '[DONE]') return false;
    let j;
    try { j = JSON.parse(payload); } catch (e) { return true; }
    const ch = j.choices && j.choices[0];
    if (!ch) return true;
    const delta = ch.delta || {}, msg = ch.message || {};
    const content = (delta.content !== undefined ? delta.content : (msg.content !== undefined ? msg.content : ''));
    if (content) { print(String(content), 'green'); finalText += content; }
    const rc = delta.reasoning_content;
    if (rc) { print(String(rc), 'gray'); finalText += rc; }
    return true;
  };
  let status = 0, ct = '', errMsg = '';
  try {
    const res = await postStream(cfg.baseUrl + '/chat/completions', headers, buildBody(cfg.model, msgs, true), (chunk) => {
      buf += chunk;
      for (;;) {
        const n = buf.indexOf('\n');
        if (n < 0) break;
        const line = buf.slice(0, n).replace(/\r$/, '');
        buf = buf.slice(n + 1);
        if (!line) continue;
        if (!handleLine(line)) return;      }
    });
    status = res.statusCode || 0;
    ct = res.headers['content-type'] || '';
    if (buf) handleLine(buf.trim());
  } catch (e) {
    println('\r\n' + L.f('cliErrHttp', (e && e.message) ? e.message : String(e)), 'red');
    return '';
  }
  if (!status || status !== 200) {
    let emsg = '';
    try { const j = JSON.parse(buf); emsg = (j.error && (j.error.message || j.error)) || ''; } catch (e) { /* ignore */ }
    println('\r\n' + L.f('cliErrStatus', status || 0, String(emsg || '')), 'red');
    return '';
  }
  if (!finalText && /json/i.test(ct)) {
    try {
      const j = JSON.parse(buf);
      const msg = j.choices && j.choices[0] && j.choices[0].message;
      if (msg && msg.content) { print(String(msg.content), 'green'); finalText = msg.content; }
    } catch (e) { /* ignore */ }
  }
  return finalText;
}

async function cliMain(L) {
  core.lineSep();
  println('  ' + L.t('p3Title'), 'cyan');
  const cfg = loadCfg();
  if (cfg.baseUrl) println(L.t('cliLoaded'), 'gray');
  if (!cfg.baseUrl) {
    println(L.t('cliSelVendor'), 'yellow');
    for (const v of VENDORS) println('  ' + v.id + '. ' + L.t(v.label));
    let no = 0;
    for (;;) {
      print(L.t('hintChoose'), 'yellow');
      const s = (await readLine()).trim();
      no = parseInt(s, 10);
      if (!isNaN(no) && no >= 1 && no <= 9) break;
      println(L.t('invalidChoice'), 'red');
    }
    const v = VENDORS[no - 1];
    cfg.vendor = v.id;
    if (!(await vendorChosen(L, v, cfg))) { println(L.t('canceled'), 'red'); return; }
    print(L.t('cliSave'), 'yellow');
    const s = (await readLine()).trim().toLowerCase();
    if (s === 'y' || s === 'yes') { saveCfg(cfg); println(L.t('cliSaved'), 'green'); }
  }
  if (!cfg.baseUrl || !cfg.model) { println(L.t('cliErrNoKey'), 'red'); return; }
  println(L.t('cliChatNote'), 'gray');
  println(L.t('cliHelp'), 'gray');
  const msgs = [];
  for (;;) {
    print('\r\n' + L.t('cliPrompt'), 'cyan');
    const line = (await readLine()).trim();
    if (line === '/exit') break;
    if (line === '/help') { println(L.t('cliHelp'), 'gray'); continue; }
    if (line === '/clear') { msgs.length = 0; println('  [cleared]', 'gray'); continue; }
    if (!line) continue;
    msgs.push(['user', line]);
    while (msgs.length > 24) msgs.shift();
    const answer = await chatOnce(L, cfg, msgs);
    print('\r\n');
    if (answer) msgs.push(['assistant', answer]);
  }
  println(L.t('cliBye'), 'green');
}
module.exports = { cliMain };