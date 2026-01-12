#include "ConfigManager.h"
#include "../Logger/Logger.h"

ConfigManager::ConfigManager() {
  _initialized = false;
  memset(&_config, 0, sizeof(DeviceConfig));
}

void ConfigManager::begin() {
  EEPROM.begin(EEPROM_SIZE);
  _initialized = true;
  
  // 尝试加载配置
  if (loadConfig(_config)) {
    Logger::info(F("ConfigMgr"), F("Loaded config from EEPROM"));
    printConfig();
  } else {
    Logger::warning(F("ConfigMgr"), F("No valid config found"));
    // 清零配置，避免显示垃圾数据
    memset(&_config, 0, sizeof(DeviceConfig));
    _config.isConfigured = false;
  }
}
bool ConfigManager::saveConfig(const DeviceConfig& config) {
  if (!_initialized) {
    Logger::error(F("ConfigMgr"), F("Not initialized"));
    return false;
  }
  
  // 创建临时配置副本并计算校验和
  DeviceConfig tempConfig = config;
  tempConfig.isConfigured = true;
  tempConfig.checksum = _calculateChecksum(tempConfig);
  
  // 写入 EEPROM
  EEPROM.put(EEPROM_START_ADDR, tempConfig);
  bool success = EEPROM.commit();
  
  if (!success) {
    Logger::error(F("ConfigMgr"), F("EEPROM commit failed"));
    return false;
  }
  
  // 等待 EEPROM 写入完成
  delay(100);
  
  // 验证写入：从 EEPROM 读取并比较
  DeviceConfig verifyConfig;
  EEPROM.get(EEPROM_START_ADDR, verifyConfig);
  
  // 验证关键字段
  bool verified = (verifyConfig.isConfigured == tempConfig.isConfigured) &&
                  (verifyConfig.checksum == tempConfig.checksum) &&
                  (strcmp(verifyConfig.ssid, tempConfig.ssid) == 0);
  
  if (verified) {
    _config = tempConfig;
    Logger::info(F("ConfigMgr"), F("Config saved and verified"));
    printConfig();
    return true;
  } else {
    Logger::error(F("ConfigMgr"), F("Config verification failed"));
    return false;
  }
}

bool ConfigManager::loadConfig(DeviceConfig& config) {
  if (!_initialized) {
    Logger::error(F("ConfigMgr"), F("Not initialized"));
    return false;
  }
  
  // 从 EEPROM 读取配置
  EEPROM.get(EEPROM_START_ADDR, config);
  
  // 验证配置
  if (!validateConfig(config)) {
    return false;
  }
  
  return true;
}

bool ConfigManager::hasValidConfig() {
  if (!_initialized) {
    return false;
  }
  
  DeviceConfig tempConfig;
  return loadConfig(tempConfig);
}

void ConfigManager::clearConfig() {
  if (!_initialized) {
    Logger::error(F("ConfigMgr"), F("Not initialized"));
    return;
  }
  
  // 清空配置
  memset(&_config, 0, sizeof(DeviceConfig));
  _config.isConfigured = false;
  
  // 写入空配置到 EEPROM
  EEPROM.put(EEPROM_START_ADDR, _config);
  EEPROM.commit();
  
  Logger::info(F("ConfigMgr"), F("Config cleared"));
}

void ConfigManager::setSSID(const char* ssid) {
  _copyString(_config.ssid, ssid, sizeof(_config.ssid));
  Logger::info(F("ConfigMgr"), F("SSID set"));
}

void ConfigManager::setPassword(const char* password) {
  _copyString(_config.password, password, sizeof(_config.password));
  Logger::info(F("ConfigMgr"), F("PASSWORD set"));
}

void ConfigManager::setMacAddress(const char* macAddress) {
  _copyString(_config.macAddress, macAddress, sizeof(_config.macAddress));
  Logger::info(F("ConfigMgr"), F("MAC address set"));
}

void ConfigManager::setAmapApiKey(const char* apiKey) {
  _copyString(_config.amapApiKey, apiKey, sizeof(_config.amapApiKey));
  Logger::info(F("ConfigMgr"), F("API key set"));
}

void ConfigManager::setCityCode(const char* cityCode) {
  _copyString(_config.cityCode, cityCode, sizeof(_config.cityCode));
  Logger::info(F("ConfigMgr"), F("City code set"));
}

DeviceConfig ConfigManager::getConfig() const {
  return _config;
}

void ConfigManager::printConfig() {
  Logger::info(F("ConfigMgr"), F("=== Device Config ==="));
  Serial.print(F("Configured: ")); Serial.println(_config.isConfigured ? F("Yes") : F("No"));
  Serial.print(F("SSID: "));
  if (_config.ssid[0]) Serial.println(_config.ssid); else Serial.println(F("Not set"));
  Serial.print(F("PASSWORD: ")); Serial.println(_config.password[0] ? F("Set") : F("Not set"));
  Serial.print(F("MAC: "));
  if (_config.macAddress[0]) Serial.println(_config.macAddress); else Serial.println(F("Not set"));
  Serial.print(F("API Key: ")); Serial.println(_config.amapApiKey[0] ? F("Set") : F("Not set"));
  Serial.print(F("City Code: "));
  if (_config.cityCode[0]) Serial.println(_config.cityCode); else Serial.println(F("Not set"));
  Serial.print(F("Checksum: 0x")); Serial.println(_config.checksum, HEX);
  Logger::info(F("ConfigMgr"), F("====================="));
}

bool ConfigManager::validateConfig(const DeviceConfig& config) {
  // 检查配置标志
  if (!config.isConfigured) {
    Logger::warning(F("ConfigMgr"), F("Config flag not set"));
    return false;
  }
  
  // 验证校验和
  uint16_t calculatedChecksum = _calculateChecksum(config);
  if (calculatedChecksum != config.checksum) {
    Logger::error(F("ConfigMgr"), F("Checksum mismatch"));
    return false;
  }
  
  // 检查必要字段是否为空
  if (strlen(config.ssid) == 0) {
    Logger::error(F("ConfigMgr"), F("SSID is empty"));
    return false;
  }
  
  return true;
}

uint16_t ConfigManager::_calculateChecksum(const DeviceConfig& config) {
  uint16_t checksum = 0;
  const uint8_t* data = (const uint8_t*)&config;
  
  // 计算除校验和字段外的所有字节
  size_t checksumOffset = offsetof(DeviceConfig, checksum);
  
  for (size_t i = 0; i < checksumOffset; i++) {
    checksum += data[i];
  }
  
  return checksum;
}

void ConfigManager::_copyString(char* dest, const char* src, size_t maxLen) {
  strncpy(dest, src, maxLen - 1);
  dest[maxLen - 1] = '\0';
}
