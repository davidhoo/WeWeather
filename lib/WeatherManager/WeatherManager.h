#ifndef WEATHER_MANAGER_H
#define WEATHER_MANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <time.h>
#include "../BM8563/BM8563.h"

// 天气信息结构体
struct WeatherInfo {
  float Temperature;    // 温度（摄氏度）
  int Humidity;         // 湿度百分比
  char Symbol;          // 天气符号字符
  String WindDirection; // 风向
  String WindSpeed;     // 风速
  String Weather;       // 天气状况
  // 天气符号映射:
  // n=晴(sunny), d=雪(snow), m=雨(rain), l=雾(fog), c=阴(overcast), o=多云(cloudy), k=雷雨(thunderstorm)
};

// 前向声明 GDEY029T94 类
class GDEY029T94;

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

class WeatherManager {
public:
  // 构造函数
  WeatherManager(const char* apiKey, const String& cityCode, BM8563* rtc, int eepromSize = 512);
  
  // 初始化
  void begin();
  
  // 获取当前天气信息
  WeatherInfo getCurrentWeather();
  
  // 更新天气信息（从网络或缓存）
  bool updateWeather(bool forceUpdate = false);
  
  // 判断是否需要从网络更新天气
  bool shouldUpdateFromNetwork();
  
  // 从网络获取天气数据
  bool fetchWeatherFromNetwork();
  
  // 从EEPROM读取天气信息
  bool readWeatherFromStorage();
  
  // 将天气信息写入EEPROM
  bool writeWeatherToStorage();
  
  // 设置更新间隔（秒）
  void setUpdateInterval(unsigned long intervalSeconds);
  
  // 获取上次更新时间
  unsigned long getLastUpdateTime();
  
  // 设置上次更新时间戳
  bool setUpdateTime(unsigned long timestamp);
  
  // 清除EEPROM中的天气数据
  void clearWeatherData();
  
  // 将天气状况映射到符号
  static char mapWeatherToSymbol(const String& weather);

private:
  // API配置
  const char* _apiKey;
  String _cityCode;
  
  // 硬件引用
  BM8563* _rtc;
  
  // EEPROM配置
  int _eepromSize;
  int _eepromAddress;
  
  // 更新间隔（默认30分钟）
  unsigned long _updateIntervalSeconds;
  
  // 当前天气信息
  WeatherInfo _currentWeather;
  
  // 私有方法
  void initializeDefaultWeather();
  void convertToStorageData(const WeatherInfo& weatherInfo, WeatherStorageData& storageData);
  void convertFromStorageData(const WeatherStorageData& storageData, WeatherInfo& weatherInfo);
  byte calculateChecksum(const WeatherStorageData& data);
  unsigned long getCurrentUnixTimestamp();
};

#endif // WEATHER_MANAGER_H