#include "WiFiManager.h"
#include "../../config.h"

extern "C" {
#include "user_interface.h"
}

WiFiManager::WiFiManager() {
  _initialized = false;
  setDefaultConfig();
}

void WiFiManager::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  _initialized = true;
  Logger::info(F("WiFiManager"), F("Initialized with default config"));
  printConfig();
}

void WiFiManager::begin(const WiFiConfig& config) {
  setConfig(config);
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  _initialized = true;
  Logger::info(F("WiFiManager"), F("Initialized with custom config"));
  printConfig();
}
void WiFiManager::setDefaultConfig() {
  // 设置默认的 WiFi 配置（从 config.h 读取）
  _copyString(_config.ssid, DEFAULT_WIFI_SSID, sizeof(_config.ssid));
  _copyString(_config.password, DEFAULT_WIFI_PASSWORD, sizeof(_config.password));
  _config.timeout = WIFI_CONNECT_TIMEOUT;
  _config.autoReconnect = true;
  _config.maxRetries = 3;
  _copyString(_config.macAddress, DEFAULT_MAC_ADDRESS, sizeof(_config.macAddress));
  _config.useMacAddress = true; // 默认启用自定义MAC地址
}

void WiFiManager::setCredentials(const char* ssid, const char* password) {
  _copyString(_config.ssid, ssid, sizeof(_config.ssid));
  _copyString(_config.password, password, sizeof(_config.password));

  String msg = "WiFi credentials updated for SSID: " + String(_config.ssid);
  Logger::info(F("WiFiManager"), msg.c_str());
}

void WiFiManager::setConfig(const WiFiConfig& config) {
  _config = config;
  Logger::info(F("WiFiManager"), F("WiFi configuration updated"));
}

WiFiConfig WiFiManager::getConfig() const {
  return _config;
}

bool WiFiManager::connect(unsigned long timeout) {
  if (!_initialized) {
    Logger::error(F("WiFiManager"), F("Not initialized. Call begin() first."));
    return false;
  }
  
  if (strlen(_config.ssid) == 0) {
    Logger::error(F("WiFiManager"), F("WiFi SSID not set. Call setCredentials() first."));
    return false;
  }
  
  // 如果启用了自定义MAC地址，先设置MAC地址
  if (_config.useMacAddress && strlen(_config.macAddress) > 0) {
    String msg = "Setting custom MAC address: " + String(_config.macAddress);
    Logger::info(F("WiFiManager"), msg.c_str());
    
    // 将MAC地址字符串转换为字节数组
    uint8_t mac[6];
    if (_parseMacAddress(_config.macAddress, mac)) {
      if (wifi_set_macaddr(STATION_IF, mac)) {
        Logger::info(F("WiFiManager"), F("MAC address set successfully"));
      } else {
        Logger::error(F("WiFiManager"), F("Failed to set MAC address"));
      }
    } else {
      Logger::warning(F("WiFiManager"), F("Invalid MAC address format, using default MAC"));
    }
  }
  
  unsigned long connectTimeout = (timeout == 0) ? _config.timeout : timeout;
  
  String msg = "Connecting to WiFi: " + String(_config.ssid);
  Logger::info(F("WiFiManager"), msg.c_str());
  WiFi.begin(_config.ssid, _config.password);
  
  return _waitForConnection(connectTimeout);
}

bool WiFiManager::scanAndConnect(unsigned long timeout) {
  if (!_initialized) {
    Logger::error(F("WiFiManager"), F("Not initialized. Call begin() first."));
    return false;
  }
  
  if (strlen(_config.ssid) == 0) {
    Logger::error(F("WiFiManager"), F("WiFi SSID not set. Call setCredentials() first."));
    return false;
  }
  
  unsigned long connectTimeout = (timeout == 0) ? _config.timeout : timeout;
  WiFi.mode(WIFI_STA);

  Logger::info(F("WiFiManager"), F("Scanning for WiFi networks..."));

  // 扫描网络
  int n = WiFi.scanNetworks();
  Logger::info(F("WiFiManager"), F("Scan done"));

  if (n == 0) {
    Logger::warning(F("WiFiManager"), F("No WiFi networks found"));
    return false;
  }

  Logger::infoValue(F("WiFiManager"), F("Networks found:"), n);
  // 查找目标网络
  for (int i = 0; i < n; ++i) {
    _printNetworkInfo(i);
    
    // 检查是否找到目标网络
    if (WiFi.SSID(i) == String(_config.ssid)) {
      String msg = "Found target network: " + String(_config.ssid);
      Logger::info(F("WiFiManager"), msg.c_str());

      // 如果配置了自定义MAC地址，则设置
      if (_config.useMacAddress && strlen(_config.macAddress) > 0) {
        String macMsg = "Setting custom MAC address: " + String(_config.macAddress);
        Logger::info(F("WiFiManager"), macMsg.c_str());

        uint8_t mac[6];
        if (_parseMacAddress(_config.macAddress, mac)) {
          // 设置MAC地址
          if (wifi_set_macaddr(STATION_IF, mac)) {
            Logger::info(F("WiFiManager"), F("MAC address set successfully"));
          } else {
            Logger::error(F("WiFiManager"), F("Failed to set MAC address"));
          }
        } else {
          Logger::warning(F("WiFiManager"), F("Invalid MAC address format, using default MAC"));
        }
      }

      // 连接到网络
      WiFi.begin(_config.ssid, _config.password);

      Logger::info(F("WiFiManager"), F("Connecting to WiFi..."));
      return _waitForConnection(connectTimeout);
    }
  }

  String msg = "Target network not found: " + String(_config.ssid);
  Logger::warning(F("WiFiManager"), msg.c_str());
  return false;
}

bool WiFiManager::autoConnect() {
  if (!_initialized) {
    Logger::error(F("WiFiManager"), F("Not initialized. Call begin() first."));
    return false;
  }
  
  int retries = 0;
  bool connected = false;
  
  while (retries < _config.maxRetries && !connected) {
    String msg = "Auto-connect attempt " + String(retries + 1) + "/" + String(_config.maxRetries);
    Logger::info(F("WiFiManager"), msg.c_str());
    
    connected = scanAndConnect();
    
    if (!connected && _config.autoReconnect) {
      retries++;
      if (retries < _config.maxRetries) {
        Logger::info(F("WiFiManager"), F("Retrying in 2 seconds..."));
        delay(2000);
      }
    } else {
      break;
    }
  }
  
  if (connected) {
    Logger::info(F("WiFiManager"), F("Auto-connect successful"));
  } else {
    String msg = "Auto-connect failed after " + String(_config.maxRetries) + " attempts";
    Logger::error(F("WiFiManager"), msg.c_str());
  }
  
  return connected;
}

bool WiFiManager::isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::disconnect() {
  WiFi.disconnect();
  Logger::info(F("WiFiManager"), F("WiFi disconnected"));
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
  String msg = "MAC address updated: " + String(_config.macAddress);
  Logger::info(F("WiFiManager"), msg.c_str());
}

String WiFiManager::getMacAddress() {
  if (_config.useMacAddress && strlen(_config.macAddress) > 0) {
    return String(_config.macAddress);
  }
  return WiFi.macAddress();
}

void WiFiManager::enableMacAddress(bool enable) {
  _config.useMacAddress = enable;
  String msg = "Custom MAC address " + String(enable ? "enabled" : "disabled");
  Logger::info(F("WiFiManager"), msg.c_str());
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
  Logger::info(F("WiFiManager"), F("=== WiFi Configuration ==="));
  String msg = "SSID: " + String(_config.ssid);
  Logger::info(F("WiFiManager"), msg.c_str());
  Logger::info(F("WiFiManager"), _config.password[0] ? F("password: ***") : F("password: Not set"));
  Logger::infoValue(F("WiFiManager"), F("Timeout:"), (int)_config.timeout, F("ms"));
  Logger::info(F("WiFiManager"), _config.autoReconnect ? F("Auto Reconnect: Enabled") : F("Auto Reconnect: Disabled"));
  Logger::infoValue(F("WiFiManager"), F("Max Retries:"), _config.maxRetries);
  msg = "MAC Address: " + String(_config.useMacAddress ? _config.macAddress : "Default");
  Logger::info(F("WiFiManager"), msg.c_str());
  Logger::info(F("WiFiManager"), _config.useMacAddress ? F("Use Custom MAC: Yes") : F("Use Custom MAC: No"));
  Logger::info(F("WiFiManager"), F("========================"));
}

void WiFiManager::_printNetworkInfo(int networkIndex) {
  String msg = String(networkIndex + 1) + ": " + WiFi.SSID(networkIndex) +
               " (" + String(WiFi.RSSI(networkIndex)) + ")" +
               ((WiFi.encryptionType(networkIndex) == ENC_TYPE_NONE) ? " " : "*");
  Logger::debug(F("WiFiManager"), msg.c_str());
}

bool WiFiManager::_waitForConnection(unsigned long timeout) {
  unsigned long startAttemptTime = millis();
  
  // 等待连接结果
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startAttemptTime < timeout) {
    delay(100);
    Serial.print(".");  // 保留进度指示
  }
  Serial.println();
  
  // 检查连接结果
  if (WiFi.status() == WL_CONNECTED) {
    Logger::info(F("WiFiManager"), F("WiFi connected successfully"));
    String msg = "IP address: " + WiFi.localIP().toString();
    Logger::info(F("WiFiManager"), msg.c_str());
    Logger::infoValue(F("WiFiManager"), F("Signal strength:"), WiFi.RSSI(), F("dBm"));
    return true;
  } else {
    Logger::error(F("WiFiManager"), F("Failed to connect to WiFi"));
    String msg = "Status: " + getStatusString();
    Logger::error(F("WiFiManager"), msg.c_str());
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