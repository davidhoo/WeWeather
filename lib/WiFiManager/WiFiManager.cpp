#include "WiFiManager.h"
#include "../../config.h"
#include "../LogManager/LogManager.h"

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
  LOG_INFO("WiFiManager initialized with default config");
  printConfig();
}

void WiFiManager::begin(const WiFiConfig& config) {
  setConfig(config);
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  _initialized = true;
  LOG_INFO("WiFiManager initialized with custom config");
  printConfig();
}

void WiFiManager::setDefaultConfig() {
  _copyString(_config.ssid, DEFAULT_WIFI_SSID, sizeof(_config.ssid));
  _copyString(_config.password, DEFAULT_WIFI_PASSWORD, sizeof(_config.password));
  _config.timeout = WIFI_CONNECT_TIMEOUT;
  _config.autoReconnect = true;
  _config.maxRetries = 3;
  _copyString(_config.macAddress, DEFAULT_MAC_ADDRESS, sizeof(_config.macAddress));
  _config.useMacAddress = ENABLE_CUSTOM_MAC;
}

void WiFiManager::setCredentials(const char* ssid, const char* password) {
  _copyString(_config.ssid, ssid, sizeof(_config.ssid));
  _copyString(_config.password, password, sizeof(_config.password));
  LogManager::info(String(F("WiFi credentials updated for SSID: ")) + _config.ssid);
}

void WiFiManager::setConfig(const WiFiConfig& config) {
  _config = config;
  LOG_INFO("WiFi configuration updated");
}

WiFiConfig WiFiManager::getConfig() const {
  return _config;
}

bool WiFiManager::connect(unsigned long timeout) {
  if (!_initialized) {
    LOG_WARN("WiFiManager not initialized. Call begin() first.");
    return false;
  }

  if (strlen(_config.ssid) == 0) {
    LOG_WARN("WiFi SSID not set. Call setCredentials() first.");
    return false;
  }

  if (_config.useMacAddress && strlen(_config.macAddress) > 0) {
    LogManager::info(String(F("Setting custom MAC address: ")) + _config.macAddress);
    uint8_t mac[6];
    if (_parseMacAddress(_config.macAddress, mac)) {
      if (wifi_set_macaddr(STATION_IF, mac)) {
        LOG_INFO("MAC address set successfully");
      } else {
        LOG_WARN("Failed to set MAC address");
      }
    } else {
      LOG_WARN("Invalid MAC address format, using default MAC");
    }
  }

  unsigned long connectTimeout = (timeout == 0) ? _config.timeout : timeout;

  LogManager::info(String(F("Connecting to WiFi: ")) + _config.ssid);
  WiFi.begin(_config.ssid, _config.password);

  return _waitForConnection(connectTimeout);
}

bool WiFiManager::scanAndConnect(unsigned long timeout) {
  if (!_initialized) {
    LOG_WARN("WiFiManager not initialized. Call begin() first.");
    return false;
  }

  if (strlen(_config.ssid) == 0) {
    LOG_WARN("WiFi SSID not set. Call setCredentials() first.");
    return false;
  }

  unsigned long connectTimeout = (timeout == 0) ? _config.timeout : timeout;

  LOG_INFO("Scanning for WiFi networks...");
  int n = WiFi.scanNetworks();
  LOG_INFO("Scan done");

  if (n == 0) {
    LOG_WARN("No WiFi networks found");
    return false;
  }

  LogManager::info(String(n) + F(" networks found"));

  for (int i = 0; i < n; ++i) {
    _printNetworkInfo(i);
    if (WiFi.SSID(i) == String(_config.ssid)) {
      LogManager::info(String(F("Found target network: ")) + _config.ssid);

      if (_config.useMacAddress && strlen(_config.macAddress) > 0) {
        LogManager::info(String(F("Setting custom MAC address: ")) + _config.macAddress);
        uint8_t mac[6];
        if (_parseMacAddress(_config.macAddress, mac)) {
          if (wifi_set_macaddr(STATION_IF, mac)) {
            LOG_INFO("MAC address set successfully");
          } else {
            LOG_WARN("Failed to set MAC address");
          }
        } else {
          LOG_WARN("Invalid MAC address format, using default MAC");
        }
      }

      WiFi.begin(_config.ssid, _config.password);
      LOG_INFO("Connecting to WiFi...");
      return _waitForConnection(connectTimeout);
    }
  }

  LogManager::warn(String(F("Target network not found: ")) + _config.ssid);
  return false;
}

bool WiFiManager::autoConnect() {
  if (!_initialized) {
    LOG_WARN("WiFiManager not initialized. Call begin() first.");
    return false;
  }

  int retries = 0;
  bool connected = false;

  while (retries < _config.maxRetries && !connected) {
    LogManager::info(String(F("Auto-connect attempt ")) + String(retries + 1) + F("/") + String(_config.maxRetries));
    connected = scanAndConnect();

    if (!connected && _config.autoReconnect) {
      retries++;
      if (retries < _config.maxRetries) {
        LOG_INFO("Retrying in 2 seconds...");
        delay(2000);
      }
    } else {
      break;
    }
  }

  if (connected) {
    LOG_INFO("Auto-connect successful");
  } else {
    LogManager::warn(String(F("Auto-connect failed after ")) + String(_config.maxRetries) + F(" attempts"));
  }

  return connected;
}

bool WiFiManager::isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::disconnect() {
  WiFi.disconnect();
  LOG_INFO("WiFi disconnected");
}

String WiFiManager::getLocalIP() {
  if (isConnected()) {
    return WiFi.localIP().toString();
  }
  return String(F("0.0.0.0"));
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
  LogManager::info(String(F("MAC address updated: ")) + _config.macAddress);
}

String WiFiManager::getMacAddress() {
  if (_config.useMacAddress && strlen(_config.macAddress) > 0) {
    return String(_config.macAddress);
  }
  return WiFi.macAddress();
}

void WiFiManager::enableMacAddress(bool enable) {
  _config.useMacAddress = enable;
  LogManager::info(String(F("Custom MAC address ")) + String(enable ? F("enabled") : F("disabled")));
}

String WiFiManager::getStatusString() {
  switch (WiFi.status()) {
    case WL_CONNECTED:
      return String(F("Connected"));
    case WL_NO_SSID_AVAIL:
      return String(F("SSID not available"));
    case WL_CONNECT_FAILED:
      return String(F("Connection failed"));
    case WL_WRONG_PASSWORD:
      return String(F("Wrong password"));
    case WL_DISCONNECTED:
      return String(F("Disconnected"));
    case WL_IDLE_STATUS:
      return String(F("Idle"));
    default:
      return String(F("Unknown status"));
  }
}

void WiFiManager::printConfig() {
  LogManager::info(F("=== WiFi Configuration ==="));
  LogManager::info(String(F("SSID: ")) + _config.ssid);
  LogManager::info(String(F("password: ")) + String(_config.password[0] ? F("***") : F("Not set")));
  LogManager::info(String(F("Timeout: ")) + String(_config.timeout) + F("ms"));
  LogManager::info(String(F("Auto Reconnect: ")) + String(_config.autoReconnect ? F("Enabled") : F("Disabled")));
  LogManager::info(String(F("Max Retries: ")) + String(_config.maxRetries));
  String macDisplay = _config.useMacAddress ? String(_config.macAddress) : String(F("Default"));
  LogManager::info(String(F("MAC Address: ")) + macDisplay);
  LogManager::info(String(F("Use Custom MAC: ")) + String(_config.useMacAddress ? F("Yes") : F("No")));
  LogManager::info(F("========================"));
}

void WiFiManager::_printNetworkInfo(int networkIndex) {
  LogManager::info(String(networkIndex + 1) + F(": ") + WiFi.SSID(networkIndex) +
                   F(" (") + String(WiFi.RSSI(networkIndex)) + F(")") +
                   String((WiFi.encryptionType(networkIndex) == ENC_TYPE_NONE) ? F(" ") : F("*")));
}

bool WiFiManager::_waitForConnection(unsigned long timeout) {
  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < timeout) {
    delay(100);
    LogManager::debug(F("."));
  }

  if (WiFi.status() == WL_CONNECTED) {
    LogManager::info(F(""));
    LOG_INFO("WiFi connected successfully");
    LogManager::info(String(F("IP address: ")) + WiFi.localIP().toString());
    LogManager::info(String(F("Signal strength: ")) + String(WiFi.RSSI()) + F(" dBm"));
    return true;
  } else {
    LogManager::info(F(""));
    LOG_WARN("Failed to connect to WiFi");
    LogManager::warn(String(F("Status: ")) + getStatusString());
    return false;
  }
}

void WiFiManager::_copyString(char* dest, const char* src, size_t maxLen) {
  strncpy(dest, src, maxLen - 1);
  dest[maxLen - 1] = '\0';
}

bool WiFiManager::_parseMacAddress(const char* macStr, uint8_t* macBytes) {
  if (strlen(macStr) != 17) {
    return false;
  }

  for (int i = 0; i < 6; i++) {
    char hex[3];
    hex[0] = macStr[i * 3];
    hex[1] = macStr[i * 3 + 1];
    hex[2] = '\0';

    if (i < 5 && macStr[i * 3 + 2] != ':') {
      return false;
    }

    char* endPtr;
    long val = strtol(hex, &endPtr, 16);
    if (*endPtr != '\0' || val < 0 || val > 255) {
      return false;
    }

    macBytes[i] = static_cast<uint8_t>(val);
  }

  return true;
}
