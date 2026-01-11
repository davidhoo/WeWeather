#include "ConfigManager.h"

ConfigManager::ConfigManager() {
  _initialized = false;
  memset(&_config, 0, sizeof(DeviceConfig));
}

void ConfigManager::begin() {
  EEPROM.begin(EEPROM_SIZE);
  _initialized = true;
  
  // 尝试加载配置
  if (loadConfig(_config)) {
    Serial.println("ConfigManager: 从 EEPROM 加载配置成功");
    printConfig();
  } else {
    Serial.println("ConfigManager: 未找到有效配置或配置损坏");
    // 清零配置，避免显示垃圾数据
    memset(&_config, 0, sizeof(DeviceConfig));
    _config.isConfigured = false;
  }
}

bool ConfigManager::saveConfig(const DeviceConfig& config) {
  if (!_initialized) {
    Serial.println("ConfigManager: 未初始化，请先调用 begin()");
    return false;
  }
  
  // 创建临时配置副本并计算校验和
  DeviceConfig tempConfig = config;
  tempConfig.isConfigured = true;
  tempConfig.checksum = _calculateChecksum(tempConfig);
  
  // 写入 EEPROM
  EEPROM.put(EEPROM_START_ADDR, tempConfig);
  bool success = EEPROM.commit();
  
  if (success) {
    _config = tempConfig;
    Serial.println("ConfigManager: 配置已保存到 EEPROM");
    printConfig();
  } else {
    Serial.println("ConfigManager: 保存配置失败");
  }
  
  return success;
}

bool ConfigManager::loadConfig(DeviceConfig& config) {
  if (!_initialized) {
    Serial.println("ConfigManager: 未初始化，请先调用 begin()");
    return false;
  }
  
  // 从 EEPROM 读取配置
  EEPROM.get(EEPROM_START_ADDR, config);
  
  // 验证配置
  if (!validateConfig(config)) {
    Serial.println("ConfigManager: 配置验证失败");
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
    Serial.println("ConfigManager: 未初始化，请先调用 begin()");
    return;
  }
  
  // 清空配置
  memset(&_config, 0, sizeof(DeviceConfig));
  _config.isConfigured = false;
  
  // 写入空配置到 EEPROM
  EEPROM.put(EEPROM_START_ADDR, _config);
  EEPROM.commit();
  
  Serial.println("ConfigManager: 配置已清除");
}

void ConfigManager::setSSID(const char* ssid) {
  _copyString(_config.ssid, ssid, sizeof(_config.ssid));
  Serial.println("ConfigManager: SSID 已设置为: " + String(_config.ssid));
}

void ConfigManager::setPassword(const char* password) {
  _copyString(_config.password, password, sizeof(_config.password));
  Serial.println("ConfigManager: 密码已设置");
}

void ConfigManager::setMacAddress(const char* macAddress) {
  _copyString(_config.macAddress, macAddress, sizeof(_config.macAddress));
  Serial.println("ConfigManager: MAC 地址已设置为: " + String(_config.macAddress));
}

void ConfigManager::setAmapApiKey(const char* apiKey) {
  _copyString(_config.amapApiKey, apiKey, sizeof(_config.amapApiKey));
  Serial.println("ConfigManager: 高德地图 API Key 已设置");
}

void ConfigManager::setCityCode(const char* cityCode) {
  _copyString(_config.cityCode, cityCode, sizeof(_config.cityCode));
  Serial.println("ConfigManager: 城市代码已设置为: " + String(_config.cityCode));
}

DeviceConfig ConfigManager::getConfig() const {
  return _config;
}

void ConfigManager::printConfig() {
  Serial.println("=== 设备配置 ===");
  Serial.println("已配置: " + String(_config.isConfigured ? "是" : "否"));
  Serial.println("SSID: " + String(_config.ssid[0] ? _config.ssid : "未设置"));
  Serial.println("密码: " + String(_config.password[0] ? _config.password : "未设置"));
  Serial.println("MAC 地址: " + String(_config.macAddress[0] ? _config.macAddress : "未设置"));
  Serial.println("高德 API Key: " + String(_config.amapApiKey[0] ? _config.amapApiKey : "未设置"));
  Serial.println("城市代码: " + String(_config.cityCode[0] ? _config.cityCode : "未设置"));
  Serial.println("校验和: 0x" + String(_config.checksum, HEX));
  Serial.println("================");
}

bool ConfigManager::validateConfig(const DeviceConfig& config) {
  // 检查配置标志
  if (!config.isConfigured) {
    Serial.println("ConfigManager: 配置标志未设置");
    return false;
  }
  
  // 验证校验和
  uint16_t calculatedChecksum = _calculateChecksum(config);
  if (calculatedChecksum != config.checksum) {
    Serial.println("ConfigManager: 校验和不匹配 (期望: 0x" + 
                   String(calculatedChecksum, HEX) + ", 实际: 0x" + 
                   String(config.checksum, HEX) + ")");
    return false;
  }
  
  // 检查必要字段是否为空
  if (strlen(config.ssid) == 0) {
    Serial.println("ConfigManager: SSID 为空");
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
