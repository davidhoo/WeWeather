#include "WiFiManager.h"

extern "C" {
#include "user_interface.h"
}

WiFiManager::WiFiManager() {
  _initialized = false;
  _webServer = nullptr;
  _dnsServer = nullptr;
  _configPortalActive = false;
  _configPortalStartTime = 0;
  setDefaultConfig();
  _setDefaultPortalConfig();
}

void WiFiManager::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  _initialized = true;
  Serial.println("WiFiManager initialized with default config");
  printConfig();
}

void WiFiManager::begin(const WiFiConfig& config) {
  setConfig(config);
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  _initialized = true;
  Serial.println("WiFiManager initialized with custom config");
  printConfig();
}

void WiFiManager::setDefaultConfig() {
  // 设置默认的 WiFi 配置
  _copyString(_config.ssid, "Sina Plaza Office", sizeof(_config.ssid));
  _copyString(_config.password, "urtheone", sizeof(_config.password));
  _config.timeout = 10000; // 10秒超时
  _config.autoReconnect = true;
  _config.maxRetries = 3;
  _copyString(_config.macAddress, "14:2B:2F:EC:0B:04", sizeof(_config.macAddress));
  _config.useMacAddress = true; // 默认启用自定义MAC地址
  _config.failureCount = 0; // 初始化失败计数
  _config.configMode = false; // 初始化配网模式状态
}

void WiFiManager::setCredentials(const char* ssid, const char* password) {
  _copyString(_config.ssid, ssid, sizeof(_config.ssid));
  _copyString(_config.password, password, sizeof(_config.password));
  
  Serial.println("WiFi credentials updated for SSID: " + String(_config.ssid));
}

void WiFiManager::setConfig(const WiFiConfig& config) {
  _config = config;
  Serial.println("WiFi configuration updated");
}

WiFiConfig WiFiManager::getConfig() const {
  return _config;
}

bool WiFiManager::connect(unsigned long timeout) {
  if (!_initialized) {
    Serial.println("WiFiManager not initialized. Call begin() first.");
    return false;
  }
  
  if (strlen(_config.ssid) == 0) {
    Serial.println("WiFi SSID not set. Call setCredentials() first.");
    return false;
  }
  
  // 如果启用了自定义MAC地址，先设置MAC地址
  if (_config.useMacAddress && strlen(_config.macAddress) > 0) {
    Serial.println("Setting custom MAC address: " + String(_config.macAddress));
    
    // 将MAC地址字符串转换为字节数组
    uint8_t mac[6];
    if (_parseMacAddress(_config.macAddress, mac)) {
      if (wifi_set_macaddr(STATION_IF, mac)) {
        Serial.println("MAC address set successfully");
      } else {
        Serial.println("Failed to set MAC address");
      }
    } else {
      Serial.println("Invalid MAC address format, using default MAC");
    }
  }
  
  unsigned long connectTimeout = (timeout == 0) ? _config.timeout : timeout;
  
  Serial.println("Connecting to WiFi: " + String(_config.ssid));
  WiFi.begin(_config.ssid, _config.password);
  
  return _waitForConnection(connectTimeout);
}

bool WiFiManager::scanAndConnect(unsigned long timeout) {
  if (!_initialized) {
    Serial.println("WiFiManager not initialized. Call begin() first.");
    return false;
  }
  
  if (strlen(_config.ssid) == 0) {
    Serial.println("WiFi SSID not set. Call setCredentials() first.");
    return false;
  }
  
  unsigned long connectTimeout = (timeout == 0) ? _config.timeout : timeout;
  
  Serial.println("Scanning for WiFi networks...");
  
  // 扫描WiFi网络
  int n = WiFi.scanNetworks();
  Serial.println("Scan done");
  
  if (n == 0) {
    Serial.println("No WiFi networks found");
    return false;
  }
  
  Serial.print(n);
  Serial.println(" networks found");
  
  // 查找目标网络
  for (int i = 0; i < n; ++i) {
    _printNetworkInfo(i);
    
    // 检查是否为目标SSID
    if (WiFi.SSID(i) == String(_config.ssid)) {
      Serial.println("Found target network: " + String(_config.ssid));
      
      // 如果启用了自定义MAC地址，先设置MAC地址
      if (_config.useMacAddress && strlen(_config.macAddress) > 0) {
        Serial.println("Setting custom MAC address: " + String(_config.macAddress));
        
        // 将MAC地址字符串转换为字节数组
        uint8_t mac[6];
        if (_parseMacAddress(_config.macAddress, mac)) {
          if (wifi_set_macaddr(STATION_IF, mac)) {
            Serial.println("MAC address set successfully");
          } else {
            Serial.println("Failed to set MAC address");
          }
        } else {
          Serial.println("Invalid MAC address format, using default MAC");
        }
      }
      
      // 连接到目标网络
      WiFi.begin(_config.ssid, _config.password);
      
      Serial.println("Connecting to WiFi...");
      return _waitForConnection(connectTimeout);
    }
  }
  
  Serial.println("Target network not found: " + String(_config.ssid));
  return false;
}

bool WiFiManager::autoConnect() {
  if (!_initialized) {
    Serial.println("WiFiManager not initialized. Call begin() first.");
    return false;
  }
  
  int retries = 0;
  bool connected = false;
  
  while (retries < _config.maxRetries && !connected) {
    Serial.println("Auto-connect attempt " + String(retries + 1) + "/" + String(_config.maxRetries));
    
    connected = scanAndConnect();
    
    if (!connected && _config.autoReconnect) {
      retries++;
      if (retries < _config.maxRetries) {
        Serial.println("Retrying in 2 seconds...");
        delay(2000);
      }
    } else {
      break;
    }
  }
  
  if (connected) {
    Serial.println("Auto-connect successful");
  } else {
    Serial.println("Auto-connect failed after " + String(_config.maxRetries) + " attempts");
  }
  
  return connected;
}

bool WiFiManager::isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::disconnect() {
  WiFi.disconnect();
  Serial.println("WiFi disconnected");
}

String WiFiManager::getLocalIP() {
  if (isConnected()) {
    return WiFi.localIP().toString();
  }
  return "0.0.0.0";
}

int WiFiManager::getRSSI() {
  if (isConnected()) {
    return WiFi.RSSI();
  }
  return 0;
}

int WiFiManager::scanNetworks() {
  return WiFi.scanNetworks();
}

String WiFiManager::getScannedSSID(int index) {
  return WiFi.SSID(index);
}

int WiFiManager::getScannedRSSI(int index) {
  return WiFi.RSSI(index);
}

bool WiFiManager::isScannedNetworkSecure(int index) {
  return WiFi.encryptionType(index) != ENC_TYPE_NONE;
}

void WiFiManager::setTimeout(unsigned long timeout) {
  _config.timeout = timeout;
}

void WiFiManager::setAutoReconnect(bool enable) {
  _config.autoReconnect = enable;
}

void WiFiManager::setMaxRetries(int retries) {
  _config.maxRetries = retries;
}

void WiFiManager::setMacAddress(const char* macAddress) {
  _copyString(_config.macAddress, macAddress, sizeof(_config.macAddress));
  Serial.println("MAC address updated: " + String(_config.macAddress));
}

String WiFiManager::getMacAddress() {
  if (_config.useMacAddress && strlen(_config.macAddress) > 0) {
    return String(_config.macAddress);
  }
  return WiFi.macAddress();
}

void WiFiManager::enableMacAddress(bool enable) {
  _config.useMacAddress = enable;
  Serial.println("Custom MAC address " + String(enable ? "enabled" : "disabled"));
}

String WiFiManager::getStatusString() {
  switch (WiFi.status()) {
    case WL_CONNECTED:
      return "Connected";
    case WL_NO_SSID_AVAIL:
      return "SSID not available";
    case WL_CONNECT_FAILED:
      return "Connection failed";
    case WL_WRONG_PASSWORD:
      return "Wrong password";
    case WL_DISCONNECTED:
      return "Disconnected";
    case WL_IDLE_STATUS:
      return "Idle";
    default:
      return "Unknown status";
  }
}

void WiFiManager::printConfig() {
  Serial.println("=== WiFi Configuration ===");
  Serial.println("SSID: " + String(_config.ssid));
  Serial.println("password: " + String(_config.password[0] ? "***" : "Not set"));
  Serial.println("Timeout: " + String(_config.timeout) + "ms");
  Serial.println("Auto Reconnect: " + String(_config.autoReconnect ? "Enabled" : "Disabled"));
  Serial.println("Max Retries: " + String(_config.maxRetries));
  Serial.println("MAC Address: " + String(_config.useMacAddress ? _config.macAddress : "Default"));
  Serial.println("Use Custom MAC: " + String(_config.useMacAddress ? "Yes" : "No"));
  Serial.println("========================");
}

void WiFiManager::_printNetworkInfo(int networkIndex) {
  Serial.print(networkIndex + 1);
  Serial.print(": ");
  Serial.print(WiFi.SSID(networkIndex));
  Serial.print(" (");
  Serial.print(WiFi.RSSI(networkIndex));
  Serial.print(")");
  Serial.println((WiFi.encryptionType(networkIndex) == ENC_TYPE_NONE) ? " " : "*");
}

bool WiFiManager::_waitForConnection(unsigned long timeout) {
  unsigned long startAttemptTime = millis();
  
  // 等待连接结果
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startAttemptTime < timeout) {
    delay(100);
    Serial.print(".");
  }
  
  // 检查连接结果
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected successfully");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    return true;
  } else {
    Serial.println("");
    Serial.println("Failed to connect to WiFi");
    Serial.println("Status: " + getStatusString());
    return false;
  }
}

void WiFiManager::_copyString(char* dest, const char* src, size_t maxLen) {
  strncpy(dest, src, maxLen - 1);
  dest[maxLen - 1] = '\0';
}

bool WiFiManager::_parseMacAddress(const char* macStr, uint8_t* macBytes) {
  // 解析MAC地址字符串 (格式: "AA:BB:CC:DD:EE:FF")
  if (strlen(macStr) != 17) {
    return false;
  }
  
  for (int i = 0; i < 6; i++) {
    char hex[3];
    hex[0] = macStr[i * 3];
    hex[1] = macStr[i * 3 + 1];
    hex[2] = '\0';
    
    // 检查分隔符
    if (i < 5 && macStr[i * 3 + 2] != ':') {
      return false;
    }
    
    // 转换十六进制字符串为字节
    char* endPtr;
    long val = strtol(hex, &endPtr, 16);
    if (*endPtr != '\0' || val < 0 || val > 255) {
      return false;
    }
    
    macBytes[i] = (uint8_t)val;
  }
  
  return true;
}

// === 配网模式相关方法实现 ===

void WiFiManager::_setDefaultPortalConfig() {
  _copyString(_portalConfig.apName, "", sizeof(_portalConfig.apName)); // 将在运行时生成
  _copyString(_portalConfig.apPassword, "", sizeof(_portalConfig.apPassword)); // 无密码
  _portalConfig.apIP = IPAddress(192, 168, 4, 1);
  _portalConfig.gateway = IPAddress(192, 168, 4, 1);
  _portalConfig.subnet = IPAddress(255, 255, 255, 0);
  _portalConfig.webServerPort = 80;
  _portalConfig.timeout = 300000; // 5分钟超时
}

bool WiFiManager::startConfigPortal() {
  String apName = _generateAPName();
  return startConfigPortal(apName.c_str());
}

bool WiFiManager::startConfigPortal(const char* apName) {
  _copyString(_portalConfig.apName, apName, sizeof(_portalConfig.apName));
  return startConfigPortal(_portalConfig);
}

bool WiFiManager::startConfigPortal(const ConfigPortalConfig& config) {
  if (_configPortalActive) {
    Serial.println("Config portal already active");
    return true;
  }
  
  _portalConfig = config;
  
  // 如果AP名称为空，生成一个
  if (strlen(_portalConfig.apName) == 0) {
    String apName = _generateAPName();
    _copyString(_portalConfig.apName, apName.c_str(), sizeof(_portalConfig.apName));
  }
  
  Serial.println("Starting config portal...");
  Serial.println("AP Name: " + String(_portalConfig.apName));
  Serial.println("AP IP: " + _portalConfig.apIP.toString());
  
  _setupConfigPortal();
  
  _configPortalActive = true;
  _configPortalStartTime = millis();
  _config.configMode = true;
  
  Serial.println("Config portal started successfully");
  Serial.println("Connect to WiFi: " + String(_portalConfig.apName));
  Serial.println("Open browser to: http://" + _portalConfig.apIP.toString());
  
  return true;
}

void WiFiManager::stopConfigPortal() {
  if (!_configPortalActive) {
    return;
  }
  
  Serial.println("Stopping config portal...");
  
  if (_webServer) {
    _webServer->stop();
    delete _webServer;
    _webServer = nullptr;
  }
  
  if (_dnsServer) {
    _dnsServer->stop();
    delete _dnsServer;
    _dnsServer = nullptr;
  }
  
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  
  _configPortalActive = false;
  _config.configMode = false;
  
  Serial.println("Config portal stopped");
}

bool WiFiManager::isConfigMode() {
  return _configPortalActive;
}

void WiFiManager::handleConfigPortal() {
  if (!_configPortalActive) {
    return;
  }
  
  // 检查超时
  if (_portalConfig.timeout > 0 &&
      millis() - _configPortalStartTime > _portalConfig.timeout) {
    Serial.println("Config portal timeout");
    stopConfigPortal();
    return;
  }
  
  // 处理DNS和Web服务器请求
  if (_dnsServer) {
    _dnsServer->processNextRequest();
  }
  
  if (_webServer) {
    _webServer->handleClient();
  }
}

String WiFiManager::getConfigPortalSSID() {
  return String(_portalConfig.apName);
}

String WiFiManager::getConfigPortalIP() {
  return _portalConfig.apIP.toString();
}

void WiFiManager::resetFailureCount() {
  _config.failureCount = 0;
  Serial.println("WiFi failure count reset");
}

int WiFiManager::getFailureCount() {
  return _config.failureCount;
}

void WiFiManager::incrementFailureCount() {
  _config.failureCount++;
  Serial.println("WiFi failure count: " + String(_config.failureCount));
}

bool WiFiManager::shouldEnterConfigMode() {
  return _config.failureCount >= 3;
}

String WiFiManager::_generateAPName() {
  // 生成随机三位数字
  randomSeed(ESP.getCycleCount());
  int randomNum = random(100, 1000);
  return "WeWeather_" + String(randomNum);
}

void WiFiManager::_setupConfigPortal() {
  // 设置AP模式
  WiFi.mode(WIFI_AP_STA);
  
  // 配置AP
  WiFi.softAPConfig(_portalConfig.apIP, _portalConfig.gateway, _portalConfig.subnet);
  
  // 启动AP
  bool apStarted;
  if (strlen(_portalConfig.apPassword) > 0) {
    apStarted = WiFi.softAP(_portalConfig.apName, _portalConfig.apPassword);
  } else {
    apStarted = WiFi.softAP(_portalConfig.apName);
  }
  
  if (!apStarted) {
    Serial.println("Failed to start AP");
    return;
  }
  
  delay(500); // 等待AP启动
  
  // 设置DNS服务器和Web服务器
  _setupDNSServer();
  _setupWebServer();
}

void WiFiManager::_setupDNSServer() {
  if (_dnsServer) {
    delete _dnsServer;
  }
  
  _dnsServer = new DNSServer();
  _dnsServer->setErrorReplyCode(DNSReplyCode::NoError);
  _dnsServer->start(53, "*", _portalConfig.apIP);
  
  Serial.println("DNS server started");
}

void WiFiManager::_setupWebServer() {
  if (_webServer) {
    delete _webServer;
  }
  
  _webServer = new ESP8266WebServer(_portalConfig.webServerPort);
  
  // 设置路由
  _webServer->on("/", [this]() { _handleRoot(); });
  _webServer->on("/wifi", HTTP_GET, [this]() { _handleRoot(); });
  _webServer->on("/wifi", HTTP_POST, [this]() { _handleWiFiSave(); });
  _webServer->on("/info", HTTP_GET, [this]() {
    String info = "AP: " + String(_portalConfig.apName) + "\n";
    info += "IP: " + _portalConfig.apIP.toString() + "\n";
    info += "MAC: " + WiFi.softAPmacAddress() + "\n";
    _webServer->send(200, "text/plain", info);
  });
  _webServer->onNotFound([this]() { _handleNotFound(); });
  
  _webServer->begin();
  Serial.println("Web server started on port " + String(_portalConfig.webServerPort));
}

void WiFiManager::_handleRoot() {
  String html = _getConfigPageHTML();
  _webServer->send(200, "text/html", html);
}

void WiFiManager::_handleWiFiSave() {
  Serial.println("Handling WiFi save request");
  
  // 获取表单数据
  String ssid = _webServer->arg("ssid");
  String password = _webServer->arg("password");
  String macAddress = _webServer->arg("mac");
  
  Serial.println("Received SSID: " + ssid);
  Serial.println("Received MAC: " + macAddress);
  
  // 验证输入
  if (ssid.length() == 0) {
    String html = _getErrorPageHTML("SSID不能为空");
    _webServer->send(400, "text/html", html);
    return;
  }
  
  if (ssid.length() > 31) {
    String html = _getErrorPageHTML("SSID长度不能超过31个字符");
    _webServer->send(400, "text/html", html);
    return;
  }
  
  if (password.length() > 63) {
    String html = _getErrorPageHTML("密码长度不能超过63个字符");
    _webServer->send(400, "text/html", html);
    return;
  }
  
  // 验证MAC地址格式（如果提供）
  if (macAddress.length() > 0 && macAddress.length() != 17) {
    String html = _getErrorPageHTML("MAC地址格式错误，应为 AA:BB:CC:DD:EE:FF");
    _webServer->send(400, "text/html", html);
    return;
  }
  
  // 保存配置
  _copyString(_config.ssid, ssid.c_str(), sizeof(_config.ssid));
  _copyString(_config.password, password.c_str(), sizeof(_config.password));
  
  if (macAddress.length() > 0) {
    _copyString(_config.macAddress, macAddress.c_str(), sizeof(_config.macAddress));
    _config.useMacAddress = true;
  } else {
    _config.useMacAddress = false;
  }
  
  // 重置失败计数
  _config.failureCount = 0;
  
  // 保存到EEPROM
  if (saveConfigToEEPROM()) {
    Serial.println("Configuration saved to EEPROM");
    
    String html = _getSuccessPageHTML();
    _webServer->send(200, "text/html", html);
    
    // 延迟后重启
    delay(2000);
    ESP.restart();
  } else {
    String html = _getErrorPageHTML("保存配置失败");
    _webServer->send(500, "text/html", html);
  }
}

void WiFiManager::_handleNotFound() {
  // 重定向到配置页面
  _webServer->sendHeader("Location", "/", true);
  _webServer->send(302, "text/plain", "");
}

String WiFiManager::_getConfigPageHTML() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>WeWeather WiFi配置</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background: #f0f0f0; }
        .container { max-width: 400px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        h1 { color: #333; text-align: center; margin-bottom: 30px; }
        .form-group { margin-bottom: 20px; }
        label { display: block; margin-bottom: 5px; color: #555; font-weight: bold; }
        input[type="text"], input[type="password"] { width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 5px; box-sizing: border-box; }
        input[type="submit"] { width: 100%; padding: 12px; background: #007bff; color: white; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; }
        input[type="submit"]:hover { background: #0056b3; }
        .info { background: #e7f3ff; padding: 15px; border-radius: 5px; margin-bottom: 20px; }
        .help { font-size: 12px; color: #666; margin-top: 5px; }
        .current-config { background: #f8f9fa; padding: 15px; border-radius: 5px; margin-bottom: 20px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🌤️ WeWeather WiFi配置</h1>
        
        <div class="info">
            <strong>当前AP信息：</strong><br>
            名称: )";
  
  html += String(_portalConfig.apName);
  html += R"(<br>
            IP地址: )";
  html += _portalConfig.apIP.toString();
  html += R"(
        </div>
        
        <div class="current-config">
            <strong>当前配置：</strong><br>
            SSID: )";
  html += String(_config.ssid);
  html += R"(<br>
            失败次数: )";
  html += String(_config.failureCount);
  html += R"(
        </div>
        
        <form method="POST" action="/wifi">
            <div class="form-group">
                <label for="ssid">WiFi名称 (SSID) *</label>
                <input type="text" id="ssid" name="ssid" required maxlength="31" value=")";
  html += String(_config.ssid);
  html += R"(">
                <div class="help">必填，最多31个字符</div>
            </div>
            
            <div class="form-group">
                <label for="password">WiFi密码</label>
                <input type="password" id="password" name="password" maxlength="63">
                <div class="help">可选，最多63个字符</div>
            </div>
            
            <div class="form-group">
                <label for="mac">自定义MAC地址</label>
                <input type="text" id="mac" name="mac" placeholder="AA:BB:CC:DD:EE:FF" maxlength="17" value=")";
  if (_config.useMacAddress) {
    html += String(_config.macAddress);
  }
  html += R"(">
                <div class="help">可选，格式: AA:BB:CC:DD:EE:FF，留空使用默认MAC</div>
            </div>
            
            <input type="submit" value="保存并重启">
        </form>
    </div>
</body>
</html>
)";
  
  return html;
}

String WiFiManager::_getSuccessPageHTML() {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>配置成功</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background: #f0f0f0; }
        .container { max-width: 400px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); text-align: center; }
        .success { color: #28a745; font-size: 18px; margin-bottom: 20px; }
        .info { background: #d4edda; padding: 15px; border-radius: 5px; margin-bottom: 20px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>✅ 配置成功</h1>
        <div class="success">WiFi配置已保存</div>
        <div class="info">
            设备将在2秒后自动重启<br>
            并尝试连接到新的WiFi网络
        </div>
        <p>如果连接失败，设备将重新进入配网模式</p>
    </div>
</body>
</html>
)";
  
  return html;
}

// === EEPROM操作方法实现 ===

bool WiFiManager::saveConfigToEEPROM() {
  EEPROM.begin(EEPROM_SIZE);
  
  // 写入配置版本标识
  EEPROM.put(CONFIG_START_ADDRESS, CONFIG_VERSION);
  
  // 写入WiFi配置
  EEPROM.put(CONFIG_START_ADDRESS + sizeof(uint32_t), _config);
  
  bool success = EEPROM.commit();
  EEPROM.end();
  
  if (success) {
    Serial.println("WiFi config saved to EEPROM");
  } else {
    Serial.println("Failed to save WiFi config to EEPROM");
  }
  
  return success;
}

bool WiFiManager::loadConfigFromEEPROM() {
  EEPROM.begin(EEPROM_SIZE);
  
  // 读取配置版本标识
  uint32_t version;
  EEPROM.get(CONFIG_START_ADDRESS, version);
  
  if (version != CONFIG_VERSION) {
    Serial.println("EEPROM config version mismatch or not found");
    EEPROM.end();
    return false;
  }
  
  // 读取WiFi配置
  WiFiConfig loadedConfig;
  EEPROM.get(CONFIG_START_ADDRESS + sizeof(uint32_t), loadedConfig);
  EEPROM.end();
  
  // 验证配置有效性
  if (!_isValidConfig(loadedConfig)) {
    Serial.println("Invalid config loaded from EEPROM");
    return false;
  }
  
  _config = loadedConfig;
  Serial.println("WiFi config loaded from EEPROM");
  Serial.println("Loaded SSID: " + String(_config.ssid));
  Serial.println("Failure count: " + String(_config.failureCount));
  
  return true;
}

void WiFiManager::clearConfigFromEEPROM() {
  EEPROM.begin(EEPROM_SIZE);
  
  // 清除配置版本标识
  EEPROM.put(CONFIG_START_ADDRESS, (uint32_t)0);
  
  EEPROM.commit();
  EEPROM.end();
  
  Serial.println("EEPROM config cleared");
}

void WiFiManager::_writeConfigToEEPROM(const WiFiConfig& config) {
  EEPROM.put(CONFIG_START_ADDRESS, CONFIG_VERSION);
  EEPROM.put(CONFIG_START_ADDRESS + sizeof(uint32_t), config);
}

bool WiFiManager::_readConfigFromEEPROM(WiFiConfig& config) {
  uint32_t version;
  EEPROM.get(CONFIG_START_ADDRESS, version);
  
  if (version != CONFIG_VERSION) {
    return false;
  }
  
  EEPROM.get(CONFIG_START_ADDRESS + sizeof(uint32_t), config);
  return _isValidConfig(config);
}

bool WiFiManager::_isValidConfig(const WiFiConfig& config) {
  // 检查SSID是否有效
  if (strlen(config.ssid) == 0 || strlen(config.ssid) > 31) {
    return false;
  }
  
  // 检查密码长度
  if (strlen(config.password) > 63) {
    return false;
  }
  
  // 检查MAC地址格式（如果启用）
  if (config.useMacAddress && strlen(config.macAddress) != 17) {
    return false;
  }
  
  // 检查其他参数的合理性
  if (config.timeout < 1000 || config.timeout > 60000) {
    return false;
  }
  
  if (config.maxRetries < 1 || config.maxRetries > 10) {
    return false;
  }
  
  return true;
}

// === 智能连接功能实现 ===

bool WiFiManager::smartConnect() {
  Serial.println("Starting smart connect...");
  
  // 首先尝试从EEPROM加载配置
  if (loadConfigFromEEPROM()) {
    Serial.println("Using saved configuration");
  } else {
    Serial.println("No saved configuration found, using default");
  }
  
  // 检查是否应该直接进入配网模式
  if (shouldEnterConfigMode()) {
    Serial.println("Failure count exceeded, entering config mode");
    return startConfigPortal();
  }
  
  // 尝试连接WiFi
  bool connected = autoConnect();
  
  if (connected) {
    Serial.println("Smart connect successful");
    resetFailureCount();
    saveConfigToEEPROM(); // 保存成功的配置
    return true;
  } else {
    Serial.println("Smart connect failed");
    incrementFailureCount();
    saveConfigToEEPROM(); // 保存失败计数
    
    // 检查是否需要进入配网模式
    if (shouldEnterConfigMode()) {
      Serial.println("Entering config mode after failures");
      return startConfigPortal();
    }
    
    return false;
  }
}

String WiFiManager::_getErrorPageHTML(const String& error) {
  String html = R"(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>配置错误</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background: #f0f0f0; }
        .container { max-width: 400px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); text-align: center; }
        .error { color: #dc3545; font-size: 18px; margin-bottom: 20px; }
        .info { background: #f8d7da; padding: 15px; border-radius: 5px; margin-bottom: 20px; }
        a { color: #007bff; text-decoration: none; }
        a:hover { text-decoration: underline; }
    </style>
</head>
<body>
    <div class="container">
        <h1>❌ 配置错误</h1>
        <div class="error">)";
  
  html += error;
  html += R"(</div>
        <div class="info">
            请检查输入信息并重试
        </div>
        <p><a href="/">返回配置页面</a></p>
    </div>
</body>
</html>
)";
  
  return html;
}