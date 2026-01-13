#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>
#include "../LogManager/LogManager.h"

// 统一配置数据结构体（用于EEPROM存储）
struct ConfigData {
  // 天气配置
  float temperature;
  int humidity;
  char symbol;
  char windDirection[16];  // 限制字符串长度
  char windSpeed[8];      // 限制字符串长度
  char weather[16];       // 限制字符串长度
  unsigned long lastUpdateTime;  // 上次更新时间戳
  
  // API配置
  char amapApiKey[64];    // 高德地图API密钥
  char cityCode[16];      // 城市代码
  
  // WiFi配置
  char wifiSSID[32];      // WiFi SSID
  char wifiPassword[32];  // WiFi密码
  
  // 硬件配置
  char macAddress[20];    // MAC地址
};

/**
 * 通用配置管理器类
 * 提供EEPROM配置存储功能，支持任意数据类型的配置存储和读取
 * 包含校验和验证机制确保配置数据完整性
 */
template<typename T>
class ConfigManager {
public:
  /**
   * 构造函数
   * @param address EEPROM起始地址
   * @param eepromSize EEPROM总大小（用于初始化）
   */
  ConfigManager(int address = 0, int eepromSize = 512);
  
  /**
   * 初始化配置管理器
   * 必须在使用前调用
   */
  void begin();
  
  /**
   * 读取配置数据
   * @param data 输出参数，读取的配置数据
   * @return 是否成功读取（校验和验证通过）
   */
  bool read(T& data);
  
  /**
   * 写入配置数据
   * @param data 要写入的配置数据
   * @return 是否成功写入
   */
  bool write(const T& data);
  
  /**
   * 清除存储的配置数据
   * 将配置区域清零并更新校验和
   */
  void clear();
  
  /**
   * 检查存储的配置数据是否有效
   * @return 配置数据是否有效（校验和验证通过）
   */
  bool isValid();
  
  /**
   * 获取配置存储地址
   * @return EEPROM存储地址
   */
  int getAddress() const;
  
  /**
   * 设置配置存储地址
   * @param address 新的EEPROM地址
   */
  void setAddress(int address);
  
  /**
   * 获取配置数据大小（包含校验和）
   * @return 总存储大小
   */
  size_t getStorageSize() const;

private:
  int _address;           // EEPROM存储地址
  int _eepromSize;        // EEPROM总大小
  bool _initialized;      // 是否已初始化
  
  /**
   * 计算配置数据的校验和
   * @param data 要计算校验和的配置数据
   * @return 校验和值
   */
  byte calculateChecksum(const T& data);
  
  /**
   * 获取校验和存储地址
   * @return 校验和在EEPROM中的地址
   */
  int getChecksumAddress() const;
};

// 模板实现必须在头文件中
template<typename T>
ConfigManager<T>::ConfigManager(int address, int eepromSize)
  : _address(address), _eepromSize(eepromSize), _initialized(false) {
}

template<typename T>
void ConfigManager<T>::begin() {
  if (!_initialized) {
    EEPROM.begin(_eepromSize);
    _initialized = true;
    LOG_INFO("ConfigManager initialized");
  }
}

template<typename T>
bool ConfigManager<T>::read(T& data) {
  if (!_initialized) {
    LOG_WARN("ConfigManager not initialized");
    return false;
  }
  
  // 从EEPROM读取配置数据
  EEPROM.get(_address, data);
  
  // 读取存储的校验和
  byte storedChecksum = EEPROM.read(getChecksumAddress());
  
  // 计算当前配置数据的校验和
  byte calculatedChecksum = calculateChecksum(data);
  
  // 验证校验和
  if (storedChecksum != calculatedChecksum) {
    LOG_ERROR("Config data checksum mismatch");
    return false;
  }
  
  LOG_INFO("Config data read successfully");
  return true;
}

template<typename T>
bool ConfigManager<T>::write(const T& data) {
  if (!_initialized) {
    LOG_WARN("ConfigManager not initialized");
    return false;
  }
  
  // 写入配置数据到EEPROM
  EEPROM.put(_address, data);
  
  // 计算并写入校验和
  byte checksum = calculateChecksum(data);
  EEPROM.write(getChecksumAddress(), checksum);
  
  // 提交更改
  bool success = EEPROM.commit();
  
  if (success) {
    LOG_INFO("Config data written successfully");
  } else {
    LOG_ERROR("Failed to write config data");
  }
  
  return success;
}
template<typename T>
void ConfigManager<T>::clear() {
  if (!_initialized) {
    LOG_WARN("ConfigManager not initialized");
    return;
  }
  
  // 创建零值配置数据
  T emptyData = {};
  
  // 写入空配置数据
  EEPROM.put(_address, emptyData);
  
  // 计算并写入校验和
  byte checksum = calculateChecksum(emptyData);
  EEPROM.write(getChecksumAddress(), checksum);
  
  // 提交更改
  EEPROM.commit();
  
  LOG_INFO("Config data cleared");
}
template<typename T>
bool ConfigManager<T>::isValid() {
  if (!_initialized) {
    return false;
  }
  
  // 读取配置数据
  T data;
  EEPROM.get(_address, data);
  
  // 读取存储的校验和
  byte storedChecksum = EEPROM.read(getChecksumAddress());
  
  // 计算当前配置数据的校验和
  byte calculatedChecksum = calculateChecksum(data);
  
  // 返回校验和是否匹配
  return storedChecksum == calculatedChecksum;
}

template<typename T>
int ConfigManager<T>::getAddress() const {
  return _address;
}

template<typename T>
void ConfigManager<T>::setAddress(int address) {
  _address = address;
}

template<typename T>
size_t ConfigManager<T>::getStorageSize() const {
  return sizeof(T) + sizeof(byte); // 配置数据大小 + 校验和大小
}

template<typename T>
byte ConfigManager<T>::calculateChecksum(const T& data) {
  byte checksum = 0;
  const byte* ptr = (const byte*)&data;
  
  for (size_t i = 0; i < sizeof(T); i++) {
    checksum ^= ptr[i];
  }
  
  return checksum;
}

template<typename T>
int ConfigManager<T>::getChecksumAddress() const {
  return _address + sizeof(T);
}

#endif // CONFIG_MANAGER_H