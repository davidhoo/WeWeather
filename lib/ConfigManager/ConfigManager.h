#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>

// EEPROM 配置结构体
struct DeviceConfig {
  char ssid[32];              // WiFi SSID
  char password[64];          // WiFi 密码
  char macAddress[18];        // MAC地址 (格式: "AA:BB:CC:DD:EE:FF")
  char amapApiKey[64];        // 高德地图 API Key
  char cityCode[16];          // 城市代码
  bool isConfigured;          // 配置标志
  uint16_t checksum;          // 校验和
};

class ConfigManager {
public:
  // 构造函数
  ConfigManager();
  
  // 初始化 ConfigManager
  void begin();
  
  // 保存配置到 EEPROM
  bool saveConfig(const DeviceConfig& config);
  
  // 从 EEPROM 加载配置
  bool loadConfig(DeviceConfig& config);
  
  // 检查是否有有效配置
  bool hasValidConfig();
  
  // 清除 EEPROM 配置
  void clearConfig();
  
  // 设置 SSID
  void setSSID(const char* ssid);
  
  // 设置密码
  void setPassword(const char* password);
  
  // 设置 MAC 地址
  void setMacAddress(const char* macAddress);
  
  // 设置高德地图 API Key
  void setAmapApiKey(const char* apiKey);
  
  // 设置城市代码
  void setCityCode(const char* cityCode);
  
  // 获取当前配置
  DeviceConfig getConfig() const;
  
  // 打印当前配置
  void printConfig();
  
  // 验证配置完整性
  bool validateConfig(const DeviceConfig& config);
  
private:
  DeviceConfig _config;
  bool _initialized;
  
  // EEPROM 起始地址
  static const int EEPROM_START_ADDR = 0;
  static const int EEPROM_SIZE = 512;
  
  // 内部辅助函数
  uint16_t _calculateChecksum(const DeviceConfig& config);
  void _copyString(char* dest, const char* src, size_t maxLen);
};

#endif // CONFIG_MANAGER_H
