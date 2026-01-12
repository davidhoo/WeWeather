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