#ifndef WEATHER_STORAGE_H
#define WEATHER_STORAGE_H

#include <Arduino.h>
#include <EEPROM.h>

// 前向声明，避免重复定义
struct WeatherInfo;

// 天气存储结构体（用于EEPROM存储）
struct WeatherStorageData {
  float temperature;
  int humidity;
  char symbol;
  char windDirection[16];  // 限制字符串长度
  char windSpeed[8];      // 限制字符串长度
  char weather[16];       // 限制字符串长度
  unsigned long lastUpdateTime;  // 上次更新时间戳
};

class WeatherStorage {
public:
  // 构造函数
  WeatherStorage(int eepromSize = 512);
  
  // 初始化EEPROM
  void begin();
  
  // 从EEPROM读取天气信息
  bool readWeatherInfo(WeatherInfo& weatherInfo);
  
  // 将天气信息写入EEPROM
  bool writeWeatherInfo(const WeatherInfo& weatherInfo);
  
  // 检查是否需要更新天气信息（基于时间间隔）
  bool shouldUpdateWeather(unsigned long intervalMs);
  
  // 获取上次更新时间
  unsigned long getLastUpdateTime();
  
  // 清除EEPROM中的天气数据
  void clearWeatherData();
  
private:
  int _eepromSize;
  int _eepromAddress;  // 存储地址
  
  // 将WeatherInfo转换为WeatherStorageData
  void convertToStorageData(const WeatherInfo& weatherInfo, WeatherStorageData& storageData);
  
  // 将WeatherStorageData转换为WeatherInfo
  void convertFromStorageData(const WeatherStorageData& storageData, WeatherInfo& weatherInfo);
  
  // 计算校验和
  byte calculateChecksum(const WeatherStorageData& data);
};

#endif // WEATHER_STORAGE_H