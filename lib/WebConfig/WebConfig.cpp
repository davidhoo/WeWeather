#include "WebConfig.h"

WebConfig::WebConfig(ConfigManager* configManager) {
  _configManager = configManager;
  _server = nullptr;
  _configMode = false;
  _configured = false;
  _configModeStartTime = 0;
}

void WebConfig::begin(const char* apSSID, const char* apPassword) {
  _apSSID = String(apSSID);
  _apPassword = String(apPassword);
  
  // 创建Web服务器实例
  if (_server) {
    delete _server;
  }
  _server = new ESP8266WebServer(80);
  
  Logger::info(F("WebConfig"), F("Initialized"));
}

bool WebConfig::enterConfigMode(unsigned long timeout) {
  Logger::info(F("WebConfig"), F("Entering config mode..."));
  
  // 停止WiFi Station模式
  WiFi.mode(WIFI_OFF);
  delay(100);
  
  // 启动AP模式
  WiFi.mode(WIFI_AP);
  
  // 配置AP
  IPAddress apIP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  
  WiFi.softAPConfig(apIP, gateway, subnet);
  
  bool apStarted;
  if (_apPassword.length() > 0) {
    apStarted = WiFi.softAP(_apSSID.c_str(), _apPassword.c_str());
  } else {
    apStarted = WiFi.softAP(_apSSID.c_str());
  }
  
  if (!apStarted) {
    Logger::error(F("WebConfig"), F("Failed to start AP"));
    return false;
  }
  
  Logger::info(F("WebConfig"), F("AP started"));
  Logger::info(F("WebConfig"), F("SSID: "));
  Serial.println(_apSSID);
  Logger::info(F("WebConfig"), F("IP: "));
  Serial.println(WiFi.softAPIP().toString());
  
  // 设置Web服务器路由
  _server->on("/", [this]() { _handleRoot(); });
  _server->on("/config", [this]() { _handleConfig(); });
  _server->on("/save", HTTP_POST, [this]() { _handleSave(); });
  _server->on("/status", [this]() { _handleStatus(); });
  _server->onNotFound([this]() { _handleNotFound(); });
  
  // 启动Web服务器
  _server->begin();
  Logger::info(F("WebConfig"), F("Web server started"));
  
  _configMode = true;
  _configured = false;
  _configModeStartTime = millis();
  
  // 阻塞式等待配置完成或超时
  while (_configMode && !_configured) {
    _server->handleClient();
    yield();
    
    // 检查超时
    if (timeout > 0 && (millis() - _configModeStartTime) > timeout) {
      Logger::warning(F("WebConfig"), F("Config mode timeout"));
      break;
    }
    
    delay(10);
  }
  
  return _configured;
}

void WebConfig::handleClient() {
  if (_server && _configMode) {
    _server->handleClient();
  }
}

void WebConfig::stop() {
  if (_server) {
    _server->stop();
    delete _server;
    _server = nullptr;
  }
  
  WiFi.softAPdisconnect(true);
  _configMode = false;
  
  Logger::info(F("WebConfig"), F("Stopped"));
}

String WebConfig::getAPIP() {
  return WiFi.softAPIP().toString();
}

String WebConfig::getAPSSID() {
  return _apSSID;
}

bool WebConfig::isConfigured() {
  return _configured;
}

void WebConfig::_handleRoot() {
  Logger::info(F("WebConfig"), F("Root page requested"));
  _server->send(200, "text/html", _generateConfigPage());
}

void WebConfig::_handleConfig() {
  Logger::info(F("WebConfig"), F("Config page requested"));
  _server->send(200, "text/html", _generateConfigPage());
}

void WebConfig::_handleSave() {
  Logger::info(F("WebConfig"), F("Save request received"));
  
  // 获取表单数据
  String ssid = _server->arg("ssid");
  String password = _server->arg("password");
  String macAddress = _server->arg("macAddress");
  String amapApiKey = _server->arg("amapApiKey");
  String cityCode = _server->arg("cityCode");
  
  Logger::info(F("WebConfig"), F("SSID: "));
  Serial.println(ssid);
  Logger::info(F("WebConfig"), F("City Code: "));
  Serial.println(cityCode);
  Logger::info(F("WebConfig"), F("MAC: "));
  Serial.println(macAddress);
  
  // 验证必填字段
  if (ssid.length() == 0 || amapApiKey.length() == 0 || cityCode.length() == 0) {
    String errorPage = F("<!DOCTYPE html><html><head><meta charset='UTF-8'><title>配置错误</title></head><body>");
    errorPage += F("<h1>配置错误</h1>");
    errorPage += F("<p>SSID、API Key 和城市代码为必填项！</p>");
    errorPage += F("<a href='/'>返回配置页面</a>");
    errorPage += F("</body></html>");
    
    _server->send(400, "text/html", errorPage);
    return;
  }
  
  // 创建配置结构体
  DeviceConfig config;
  memset(&config, 0, sizeof(DeviceConfig));
  
  // 设置配置数据
  strncpy(config.ssid, ssid.c_str(), sizeof(config.ssid) - 1);
  strncpy(config.password, password.c_str(), sizeof(config.password) - 1);
  strncpy(config.macAddress, macAddress.c_str(), sizeof(config.macAddress) - 1);
  strncpy(config.amapApiKey, amapApiKey.c_str(), sizeof(config.amapApiKey) - 1);
  strncpy(config.cityCode, cityCode.c_str(), sizeof(config.cityCode) - 1);
  
  // 保存配置到EEPROM
  if (_configManager->saveConfig(config)) {
    Logger::info(F("WebConfig"), F("Config saved successfully"));
    _server->send(200, "text/html", _generateSuccessPage());
    _configured = true;
    _configMode = false;
  } else {
    Logger::error(F("WebConfig"), F("Failed to save config"));
    String errorPage = F("<!DOCTYPE html><html><head><meta charset='UTF-8'><title>保存失败</title></head><body>");
    errorPage += F("<h1>保存失败</h1>");
    errorPage += F("<p>配置保存到EEPROM失败，请重试。</p>");
    errorPage += F("<a href='/'>返回配置页面</a>");
    errorPage += F("</body></html>");
    
    _server->send(500, "text/html", errorPage);
  }
}

void WebConfig::_handleStatus() {
  Logger::info(F("WebConfig"), F("Status page requested"));
  _server->send(200, "text/html", _generateStatusPage());
}

void WebConfig::_handleNotFound() {
  Logger::warning(F("WebConfig"), F("404: "));
  Serial.println(_server->uri());
  _server->send(404, "text/plain", "Not Found");
}

String WebConfig::_generateConfigPage() {
  // 获取当前配置
  DeviceConfig currentConfig = _configManager->getConfig();
  
  String page = F("<!DOCTYPE html>");
  page += F("<html><head>");
  page += F("<meta charset='UTF-8'>");
  page += F("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
  page += F("<title>WeWeather 配置</title>");
  page += F("<style>");
  page += F("body { font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }");
  page += F(".container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }");
  page += F("h1 { color: #333; text-align: center; margin-bottom: 30px; }");
  page += F(".form-group { margin-bottom: 20px; }");
  page += F("label { display: block; margin-bottom: 5px; font-weight: bold; color: #555; }");
  page += F("input[type='text'], input[type='password'] { width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 4px; font-size: 16px; box-sizing: border-box; }");
  page += F("input[type='submit'] { background-color: #007bff; color: white; padding: 12px 30px; border: none; border-radius: 4px; cursor: pointer; font-size: 16px; width: 100%; }");
  page += F("input[type='submit']:hover { background-color: #0056b3; }");
  page += F(".info { background-color: #e7f3ff; padding: 15px; border-radius: 4px; margin-bottom: 20px; }");
  page += F(".required { color: red; }");
  page += F("</style>");
  page += F("</head><body>");
  
  page += F("<div class='container'>");
  page += F("<h1>WeWeather 设备配置</h1>");
  
  page += F("<div class='info'>");
  page += F("<strong>设备信息:</strong><br>");
  page += F("热点名称: ") + _htmlEncode(_apSSID) + F("<br>");
  page += F("IP地址: ") + getAPIP();
  page += F("</div>");
  
  page += F("<form action='/save' method='post'>");
  
  page += F("<div class='form-group'>");
  page += F("<label for='ssid'>WiFi名称 (SSID) <span class='required'>*</span></label>");
  page += F("<input type='text' id='ssid' name='ssid' value='") + _htmlEncode(currentConfig.ssid) + F("' required>");
  page += F("</div>");
  
  page += F("<div class='form-group'>");
  page += F("<label for='password'>WiFi密码</label>");
  page += F("<input type='text' id='password' name='password' value='") + _htmlEncode(currentConfig.password) + F("'>");
  page += F("</div>");
  
  page += F("<div class='form-group'>");
  page += F("<label for='amapApiKey'>高德地图API Key <span class='required'>*</span></label>");
  page += F("<input type='text' id='amapApiKey' name='amapApiKey' value='") + _htmlEncode(currentConfig.amapApiKey) + F("' required>");
  page += F("</div>");
  
  page += F("<div class='form-group'>");
  page += F("<label for='cityCode'>城市代码 <span class='required'>*</span></label>");
  page += F("<input type='text' id='cityCode' name='cityCode' value='") + _htmlEncode(currentConfig.cityCode) + F("' required>");
  page += F("<small>例如: 110108 (北京海淀区)</small>");
  page += F("</div>");
  
  page += F("<div class='form-group'>");
  page += F("<label for='macAddress'>MAC地址 (可选)</label>");
  page += F("<input type='text' id='macAddress' name='macAddress' value='") + _htmlEncode(currentConfig.macAddress) + F("' placeholder='AA:BB:CC:DD:EE:FF'>");
  page += F("<small>留空则使用硬件默认MAC地址</small>");
  page += F("</div>");
  
  page += F("<input type='submit' value='保存配置'>");
  page += F("</form>");
  
  page += F("</div>");
  page += F("</body></html>");
  
  return page;
}

String WebConfig::_generateSuccessPage() {
  String page = F("<!DOCTYPE html>");
  page += F("<html><head>");
  page += F("<meta charset='UTF-8'>");
  page += F("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
  page += F("<title>配置成功</title>");
  page += F("<style>");
  page += F("body { font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }");
  page += F(".container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); text-align: center; }");
  page += F("h1 { color: #28a745; }");
  page += F(".success { background-color: #d4edda; color: #155724; padding: 15px; border-radius: 4px; margin: 20px 0; }");
  page += F("</style>");
  page += F("</head><body>");
  
  page += F("<div class='container'>");
  page += F("<h1>✓ 配置保存成功</h1>");
  page += F("<div class='success'>");
  page += F("配置已成功保存到设备中。<br>");
  page += F("设备将在几秒钟后自动重启并应用新配置。");
  page += F("</div>");
  page += F("</div>");
  
  page += F("<script>");
  page += F("setTimeout(function() { window.close(); }, 3000);");
  page += F("</script>");
  
  page += F("</body></html>");
  
  return page;
}

String WebConfig::_generateStatusPage() {
  DeviceConfig config = _configManager->getConfig();
  
  String page = F("<!DOCTYPE html>");
  page += F("<html><head>");
  page += F("<meta charset='UTF-8'>");
  page += F("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
  page += F("<title>设备状态</title>");
  page += F("<style>");
  page += F("body { font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }");
  page += F(".container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }");
  page += F("h1 { color: #333; text-align: center; }");
  page += F(".status-item { margin: 10px 0; padding: 10px; background-color: #f8f9fa; border-radius: 4px; }");
  page += F("</style>");
  page += F("</head><body>");
  
  page += F("<div class='container'>");
  page += F("<h1>设备状态</h1>");
  
  page += F("<div class='status-item'>");
  page += F("<strong>配置状态:</strong> ");
  page += config.isConfigured ? F("已配置") : F("未配置");
  page += F("</div>");
  
  page += F("<div class='status-item'>");
  page += F("<strong>WiFi SSID:</strong> ") + _htmlEncode(config.ssid);
  page += F("</div>");
  
  page += F("<div class='status-item'>");
  page += F("<strong>城市代码:</strong> ") + _htmlEncode(config.cityCode);
  page += F("</div>");
  
  page += F("<div class='status-item'>");
  page += F("<strong>MAC地址:</strong> ") + _htmlEncode(config.macAddress);
  page += F("</div>");
  
  page += F("<div class='status-item'>");
  page += F("<strong>API Key:</strong> ");
  page += (strlen(config.amapApiKey) > 0 ? F("已设置") : F("未设置"));
  page += F("</div>");
  
  page += F("<p><a href='/'>返回配置页面</a></p>");
  
  page += F("</div>");
  page += F("</body></html>");
  
  return page;
}

String WebConfig::_htmlEncode(const String& str) {
  String encoded = str;
  encoded.replace("&", "&amp;");
  encoded.replace("<", "&lt;");
  encoded.replace(">", "&gt;");
  encoded.replace("\"", "&quot;");
  encoded.replace("'", "&#39;");
  return encoded;
}