const char *CONFIG_HTML = R"rawliteral(
<!DOCTYPE html>
<html lang="zh">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Panel Config</title>
<style>
body{background:#1a1a2e;color:#e0e0e0;font-family:monospace;margin:0;padding:16px}
h2{text-align:center;color:#f0a500;margin-bottom:20px;font-size:18px}
.b{background:#16213e;border:1px solid #0f3460;border-radius:8px;padding:12px;margin-bottom:12px}
.b h3{margin:0 0 8px;color:#e94560;font-size:14px}
.f{display:flex;align-items:center;margin-bottom:8px}
.f label{width:72px;font-size:12px;color:#aaa}
.f input,.f select{flex:1;background:#0f3460;border:1px solid #533483;color:#fff;padding:6px;border-radius:4px;font-size:13px}
.f input:focus,.f select:focus{outline:none;border-color:#f0a500}
.a{display:flex;gap:10px;margin-top:16px}
button{flex:1;padding:10px;border:none;border-radius:6px;font-size:14px;cursor:pointer}
#s{background:#f0a500;color:#000;font-weight:bold}
#r{background:#533483;color:#fff}
#st{text-align:center;margin-top:10px;font-size:12px;min-height:18px}
</style>
</head>
<body>
<h2>Panel Button Config</h2>
<div id="rows"></div>
<div class="a">
  <button id="s">Save &amp; Reboot</button>
  <button id="r">Reset</button>
</div>
<div id="st"></div>
<script>
var IC=["lock","lock_open","lightbulb","fan","sun","moon",
"curtains","gate","garage","door","camera","thermometer",
"volume_high","music_note","calendar","clock",
"bell","power","settings","home","arrow_right",
"alert_circle","chevron_left","reboot","close","wifi"];
var TP=["switch","light","sensor"];
var B=[];
function esc(s){return String(s||'').replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;')}
function render(){
  var c=document.getElementById('rows');c.innerHTML='';
  for(var i=0;i<4;i++){
    var b=B[i]||{entity_id:'',label:'Button '+(i+1),icon:'lightbulb',type:'switch'};
    var d=document.createElement('div');d.className='b';
    d.innerHTML='<h3>Button '+(i+1)+'</h3>'+
      '<div class="f"><label>Type</label>'+
      '<select id="tp'+i+'">'+TP.map(function(t){return '<option value="'+esc(t)+'"'+(t===b.type?' selected':'')+'>'+esc(t)+'</option>'}).join('')+'</select></div>'+
      '<div class="f"><label>Entity</label><input id="e'+i+'" value="'+esc(b.entity_id)+'" placeholder="e.g. light.kitchen"></div>'+
      '<div class="f"><label>Label</label><input id="l'+i+'" value="'+esc(b.label)+'"></div>'+
      '<div class="f"><label>Icon</label>'+
      '<select id="ic'+i+'">'+IC.map(function(ic){return '<option value="'+esc(ic)+'"'+(ic===b.icon?' selected':'')+'>'+esc(ic)+'</option>'}).join('')+'</select></div>';
    c.appendChild(d);
  }
}
function load(){
  fetch('/buttons.json').then(function(r){return r.json();}).then(function(d){
    B=d.buttons||[];render();
  }).catch(function(e){document.getElementById('st').innerHTML='<span style="color:red">Load error</span>';});
}
document.getElementById('s').onclick=function(){
  var a=[];
  for(var i=0;i<4;i++) a.push({
    entity_id:document.getElementById('e'+i).value,
    label:document.getElementById('l'+i).value,
    icon:document.getElementById('ic'+i).value,
    type:document.getElementById('tp'+i).value
  });
  document.getElementById('st').innerHTML='<span style="color:#f0a500">Saving...</span>';
  var xhr=new XMLHttpRequest();
  xhr.open('POST','/save',true);
  xhr.setRequestHeader('Content-Type','application/json');
  xhr.onload=function(){
    if(xhr.status===200) document.getElementById('st').innerHTML='Saved! Rebooting...';
    else document.getElementById('st').innerHTML='<span style="color:red">Save failed ('+xhr.status+')</span>';
  };
  xhr.send(JSON.stringify({buttons:a}));
};
document.getElementById('r').onclick=function(){if(confirm('Reset all buttons to defaults?'))load();};
load();
</script>
</body>
</html>
)rawliteral";

const char *DEFAULT_CONFIG = "{\"buttons\":["
  "{\"entity_id\":\"\",\"label\":\"Button 1\",\"icon\":\"lightbulb\",\"type\":\"switch\"},"
  "{\"entity_id\":\"\",\"label\":\"Button 2\",\"icon\":\"fan\",\"type\":\"switch\"},"
  "{\"entity_id\":\"\",\"label\":\"Button 3\",\"icon\":\"light\",\"type\":\"switch\"},"
  "{\"entity_id\":\"\",\"label\":\"Button 4\",\"icon\":\"settings\",\"type\":\"switch\"}"
"]}";

static const struct { const char *name; const char *hex; } ICON_MAP[] = {
  {"lock","01C0"},{"lock_open","01C1"},{"lightbulb","005E"},{"fan","026B"},
  {"sun","00DA"},{"moon","00DB"},{"curtains","0B8A"},{"gate","07CD"},
  {"garage","07CE"},{"door","07CF"},{"camera","01DC"},{"thermometer","01DB"},
  {"volume_high","03D9"},{"music_note","06CB"},{"calendar","023A"},
  {"clock","023F"},{"bell","00E6"},{"power","04A1"},{"settings","059F"},
  {"home","0051"},{"arrow_right","005F"},{"alert_circle","005D"},
  {"chevron_left","0061"},{"reboot","0BAE"},{"close","0062"},{"wifi","03C5"},
};

#include "config_manager.h"
#include <memory>

using namespace esphome;
using namespace esphome::config_manager;

std::string ConfigManagerComponent::get_icon_hex(const std::string &name) const {
  for (auto &e : ICON_MAP)
    if (std::string(e.name) == name) return "\\U000F" + std::string(e.hex);
  return "\\U000F005E";
}

void ConfigManagerComponent::setup() {
  ESP_LOGI(TAG, "ConfigManager starting...");
#ifdef USE_esp32
  if (!LittleFS.begin(true)) {
    ESP_LOGE(TAG, "LittleFS mount failed");
    return;
  }
  if (!LittleFS.exists("/buttons.json")) {
    write_config(DEFAULT_CONFIG);
    ESP_LOGI(TAG, "Created default config");
  }
  std::string json = read_config();
  if (!json.empty()) parse_config(json);
  else for (int i = 0; i < 4; i++)
    buttons_.push_back({"", "Button " + std::to_string(i+1), "lightbulb", "switch"});

  ESP_LOGI(TAG, "Loaded %zu buttons", buttons_.size());
  for (size_t i = 0; i < buttons_.size(); i++)
    ESP_LOGI(TAG, "  Btn %zu: type=%s label='%s' icon=%s entity='%s'",
             i, buttons_[i].type.c_str(), buttons_[i].label.c_str(),
             get_icon_hex(buttons_[i].icon).c_str(),
             buttons_[i].entity_id.c_str());

  server_ = std::make_unique<AsyncWebServer>(80);
  server_->on("/", HTTP_GET, [this](AsyncWebServerRequest *r){ handle_root(r); });
  server_->on("/buttons.json", HTTP_GET, [this](AsyncWebServerRequest *r){ handle_json(r); });
  server_->on("/save", HTTP_POST,
    [this](AsyncWebServerRequest *r){ handle_save(r, nullptr, 0, 0, 0); },
    [this](AsyncWebServerRequest *r, uint8_t *d, size_t len, size_t idx, size_t tot){
      if(idx < tot) return;
      handle_save(r, d, len, idx, tot);
    });
  server_->begin();
  ESP_LOGI(TAG, "Web server: http://%s.local/", App.get_name().c_str());
#endif
}

void ConfigManagerComponent::loop() {}

void ConfigManagerComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "ConfigManager - http://<device-ip>/");
}

#ifdef USE_esp32
void ConfigManagerComponent::handle_root(AsyncWebServerRequest *r) {
  r->send(200, "text/html; charset=utf-8", CONFIG_HTML);
}

void ConfigManagerComponent::handle_json(AsyncWebServerRequest *r) {
  std::string j = read_config();
  if (j.empty()) j = DEFAULT_CONFIG;
  r->send(200, "application/json", j);
}

void ConfigManagerComponent::handle_save(AsyncWebServerRequest *r,
                                          uint8_t *data, size_t len,
                                          size_t idx, size_t tot) {
  if (idx < tot) return;
  std::string body(reinterpret_cast<char*>(data), len);
  if (body.find("\"buttons\"") == std::string::npos) {
    r->send(400, "text/plain", "Invalid JSON"); return;
  }
  if (!write_config(body) || !parse_config(body)) {
    ESP_LOGE(TAG, "Failed to save config");
    r->send(500, "text/plain", "Failed"); return;
  }
  ESP_LOGI(TAG, "Config saved (%zu bytes), rebooting...", body.size());
  r->send(200, "text/plain", "OK");
  delay(500);
  ESP.restart();
}
#endif

std::string ConfigManagerComponent::read_config() {
  std::string r;
  File f = LittleFS.open("/buttons.json", "r");
  if (f) { while(f.available()) r += (char)f.read(); f.close(); }
  return r;
}

bool ConfigManagerComponent::write_config(const std::string &j) {
  File f = LittleFS.open("/buttons.json", "w");
  if (!f) return false;
  f.print(j); f.close(); return true;
}

bool ConfigManagerComponent::parse_config(const std::string &j) {
  buttons_.clear();
  size_t p = j.find("\"buttons\"");
  if (p == std::string::npos) return false;
  p = j.find('[', p);
  if (p == std::string::npos) return false;
  size_t e = j.rfind(']');
  std::string arr = j.substr(p+1, e-p-1);
  auto ex = [&](const std::string &s, const std::string &k) -> std::string {
    std::string kk = "\"" + k + "\"";
    size_t kp = s.find(kk);
    if (kp == std::string::npos) return "";
    kp = s.find('"', kp + kk.length());
    if (kp == std::string::npos) return "";
    kp++;
    size_t ep = s.find('"', kp);
    return ep == std::string::npos ? "" : s.substr(kp, ep - kp);
  };
  size_t i = 0;
  while (i < arr.size() && buttons_.size() < 4) {
    size_t os = arr.find('{', i);
    if (os == std::string::npos) break;
    size_t oe = arr.find('}', os);
    if (oe == std::string::npos) break;
    std::string obj = arr.substr(os + 1, oe - os - 1);
    std::string type = ex(obj, "type");
    if (type != "switch" && type != "light" && type != "sensor")
      type = "switch";
    buttons_.push_back({ex(obj,"entity_id"), ex(obj,"label"),
                        ex(obj,"icon"), type});
    i = oe + 1;
  }
  while (buttons_.size() < 4)
    buttons_.push_back({"", "Button " + std::to_string(buttons_.size() + 1),
                        "lightbulb", "switch"});
  return true;
}
