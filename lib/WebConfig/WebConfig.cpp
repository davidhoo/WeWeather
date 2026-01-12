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
  
  Serial.println("\n=== 启动 Web 配置模式 ===");
  
  // 断开现有 WiFi 连接
  WiFi.disconnect();
  delay(100);
  
  // 启动 AP 模式
  WiFi.mode(WIFI_AP);
  
  bool apStarted;
  if (_apPassword.length() > 0) {
    apStarted = WiFi.softAP(_apSSID.c_str(), _apPassword.c_str());
  } else {
    apStarted = WiFi.softAP(_apSSID.c_str());
  }
  
  if (apStarted) {
    Serial.println("AP 启动成功");
    Serial.println("SSID: " + _apSSID);
    Serial.println("IP 地址: " + WiFi.softAPIP().toString());
  } else {
    Serial.println("AP 启动失败");
    return;
  }
  
  // 创建 Web 服务器
  _server = new ESP8266WebServer(80);
  
  // 设置路由
  _server->on("/", [this]() { _handleRoot(); });
  _server->on("/config", [this]() { _handleConfig(); });
  _server->on("/save", HTTP_POST, [this]() { _handleSave(); });
  _server->on("/status", [this]() { _handleStatus(); });
  _server->onNotFound([this]() { _handleNotFound(); });
  
  // 启动服务器
  _server->begin();
  Serial.println("Web 服务器已启动");
  Serial.println("请连接到 WiFi: " + _apSSID);
  Serial.println("然后访问: http://" + WiFi.softAPIP().toString());
  Serial.println("========================\n");
}

bool WebConfig::enterConfigMode(unsigned long timeout) {
  _configMode = true;
  _configured = false;
  _configModeStartTime = millis();
  
  Serial.println("进入 Web 配置模式，等待配置...");
  Serial.println("超时时间: " + String(timeout / 1000) + " 秒");
  
  // 阻塞式等待配置
  while (_configMode && !_configured && (millis() - _configModeStartTime < timeout)) {
    handleClient();
    delay(10);
  }
  
  if (_configured) {
    Serial.println("\n配置已完成");
    return true;
  } else if (millis() - _configModeStartTime >= timeout) {
    Serial.println("\n配置模式超时");
    _configMode = false;
    return false;
  }
  
  return false;
}

void WebConfig::handleClient() {
  if (_server) {
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
  
  Serial.println("Web 配置模式已停止");
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
  Serial.println("收到根路径请求");
  _server->send(200, "text/html", _generateConfigPage());
}

void WebConfig::_handleConfig() {
  Serial.println("收到配置页面请求");
  _server->send(200, "text/html", _generateConfigPage());
}

void WebConfig::_handleSave() {
  Serial.println("收到保存配置请求");
  
  // 获取表单数据
  String ssid = _server->arg("ssid");
  String password = _server->arg("password");
  String macAddress = _server->arg("mac");
  String apiKey = _server->arg("apikey");
  String cityCode = _server->arg("citycode");
  
  Serial.println("接收到的配置:");
  Serial.println("  SSID: " + ssid);
  Serial.println("  密码: " + String(password.length() > 0 ? "******" : "未设置"));
  Serial.println("  MAC: " + macAddress);
  Serial.println("  API Key: " + String(apiKey.length() > 0 ? "已设置" : "未设置"));
  Serial.println("  城市代码: " + cityCode);
  
  // 验证必填字段
  if (ssid.length() == 0) {
    _server->send(400, "text/html", 
      "<html><body><h1>错误</h1><p>SSID 不能为空</p>"
      "<a href='/'>返回</a></body></html>");
    return;
  }
  
  // 设置配置
  _configManager->setSSID(ssid.c_str());
  
  if (password.length() > 0) {
    _configManager->setPassword(password.c_str());
  }
  
  if (macAddress.length() > 0) {
    _configManager->setMacAddress(macAddress.c_str());
  }
  
  if (apiKey.length() > 0) {
    _configManager->setAmapApiKey(apiKey.c_str());
  }
  
  if (cityCode.length() > 0) {
    _configManager->setCityCode(cityCode.c_str());
  }
  
  // 保存配置
  DeviceConfig config = _configManager->getConfig();
  if (_configManager->saveConfig(config)) {
    Serial.println("配置保存成功");
    _configured = true;
    _server->send(200, "text/html", _generateSuccessPage());
  } else {
    Serial.println("配置保存失败");
    _server->send(500, "text/html", 
      "<html><body><h1>错误</h1><p>配置保存失败</p>"
      "<a href='/'>返回</a></body></html>");
  }
}

void WebConfig::_handleStatus() {
  Serial.println("收到状态查询请求");
  _server->send(200, "text/html", _generateStatusPage());
}

void WebConfig::_handleNotFound() {
  Serial.println("收到未知路径请求: " + _server->uri());
  _server->send(404, "text/html", 
    "<html><body><h1>404 Not Found</h1>"
    "<a href='/'>返回首页</a></body></html>");
}

String WebConfig::_generateConfigPage() {
  // 获取当前配置
  DeviceConfig config = _configManager->getConfig();
  
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>WeWeather 配置</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; max-width: 600px; margin: 50px auto; padding: 20px; background: #f0f0f0; }";
  html += "h1 { color: #333; text-align: center; }";
  html += ".container { background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }";
  html += ".form-group { margin-bottom: 20px; }";
  html += "label { display: block; margin-bottom: 5px; color: #555; font-weight: bold; }";
  html += "input[type='text'], input[type='password'] { width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 5px; box-sizing: border-box; }";
  html += "input[type='submit'] { width: 100%; padding: 12px; background: #4CAF50; color: white; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; }";
  html += "input[type='submit']:hover { background: #45a049; }";
  html += ".info { background: #e7f3fe; padding: 15px; border-left: 4px solid #2196F3; margin-bottom: 20px; }";
  html += ".required { color: red; }";
  html += ".hint { font-size: 12px; color: #888; margin-top: 5px; }";
  html += "</style>";
  html += "</head><body>";
  html += "<div class='container'>";
  html += "<h1>⚙️ WeWeather 配置</h1>";
  html += "<div class='info'>请填写以下配置信息。标记 <span class='required'>*</span> 的为必填项。</div>";
  html += "<form action='/save' method='POST'>";
  
  // WiFi SSID
  html += "<div class='form-group'>";
  html += "<label>WiFi SSID <span class='required'>*</span></label>";
  html += "<input type='text' name='ssid' value='" + _htmlEncode(String(config.ssid)) + "' required>";
  html += "<div class='hint'>要连接的 WiFi 网络名称</div>";
  html += "</div>";
  
  // WiFi 密码
  html += "<div class='form-group'>";
  html += "<label>WiFi 密码</label>";
  html += "<input type='text' name='password' value='" + _htmlEncode(String(config.password)) + "'>";
  html += "<div class='hint'>WiFi 网络密码</div>";
  html += "</div>";
  
  // MAC 地址
  html += "<div class='form-group'>";
  html += "<label>MAC 地址</label>";
  html += "<input type='text' name='mac' value='" + _htmlEncode(String(config.macAddress)) + "' placeholder='AA:BB:CC:DD:EE:FF'>";
  html += "<div class='hint'>格式: AA:BB:CC:DD:EE:FF（可选）</div>";
  html += "</div>";
  
  // 高德地图 API Key
  html += "<div class='form-group'>";
  html += "<label>高德地图 API Key</label>";
  html += "<input type='text' name='apikey' value='" + _htmlEncode(String(config.amapApiKey)) + "'>";
  html += "<div class='hint'>用于获取天气信息</div>";
  html += "</div>";
  
  // 城市代码
  html += "<div class='form-group'>";
  html += "<label>城市代码</label>";
  html += "<input type='text' name='citycode' value='" + _htmlEncode(String(config.cityCode)) + "' placeholder='110108'>";
  html += "<div class='hint'>高德地图城市代码（如: 110108 为北京海淀区）</div>";
  html += "</div>";
  
  html += "<input type='submit' value='保存配置'>";
  html += "</form>";
  html += "<div style='text-align: center; margin-top: 20px;'>";
  html += "<a href='/status'>查看当前配置</a>";
  html += "</div>";
  html += "</div>";
  html += "</body></html>";
  
  return html;
}

String WebConfig::_generateSuccessPage() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>配置成功</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; max-width: 600px; margin: 50px auto; padding: 20px; background: #f0f0f0; }";
  html += ".container { background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); text-align: center; }";
  html += "h1 { color: #4CAF50; }";
  html += ".success-icon { font-size: 64px; color: #4CAF50; margin: 20px 0; }";
  html += "p { color: #555; line-height: 1.6; }";
  html += "</style>";
  html += "</head><body>";
  html += "<div class='container'>";
  html += "<div class='success-icon'>✓</div>";
  html += "<h1>配置保存成功！</h1>";
  html += "<p>您的配置已成功保存到设备。</p>";
  html += "<p>设备将在几秒钟后重启并应用新配置。</p>";
  html += "</div>";
  html += "</body></html>";
  
  return html;
}

String WebConfig::_generateStatusPage() {
  DeviceConfig config = _configManager->getConfig();
  
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>当前配置</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; max-width: 600px; margin: 50px auto; padding: 20px; background: #f0f0f0; }";
  html += ".container { background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }";
  html += "h1 { color: #333; text-align: center; }";
  html += "table { width: 100%; border-collapse: collapse; margin: 20px 0; }";
  html += "th, td { padding: 12px; text-align: left; border-bottom: 1px solid #ddd; }";
  html += "th { background: #f5f5f5; font-weight: bold; }";
  html += ".back-link { text-align: center; margin-top: 20px; }";
  html += "a { color: #2196F3; text-decoration: none; }";
  html += "a:hover { text-decoration: underline; }";
  html += "</style>";
  html += "</head><body>";
  html += "<div class='container'>";
  html += "<h1>📋 当前配置</h1>";
  html += "<table>";
  html += "<tr><th>配置项</th><th>值</th></tr>";
  html += "<tr><td>已配置</td><td>" + String(config.isConfigured ? "是" : "否") + "</td></tr>";
  html += "<tr><td>WiFi SSID</td><td>" + _htmlEncode(String(config.ssid[0] ? config.ssid : "未设置")) + "</td></tr>";
  html += "<tr><td>WiFi 密码</td><td>" + _htmlEncode(String(config.password[0] ? config.password : "未设置")) + "</td></tr>";
  html += "<tr><td>MAC 地址</td><td>" + _htmlEncode(String(config.macAddress[0] ? config.macAddress : "未设置")) + "</td></tr>";
  html += "<tr><td>API Key</td><td>" + _htmlEncode(String(config.amapApiKey[0] ? config.amapApiKey : "未设置")) + "</td></tr>";
  html += "<tr><td>城市代码</td><td>" + _htmlEncode(String(config.cityCode[0] ? config.cityCode : "未设置")) + "</td></tr>";
  html += "</table>";
  html += "<div class='back-link'><a href='/'>返回配置页面</a></div>";
  html += "</div>";
  html += "</body></html>";
  
  return html;
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
