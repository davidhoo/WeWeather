#ifndef WEATHER_MANAGER_H
#define WEATHER_MANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <time.h>
#include "../BM8563/BM8563.h"
#include "../ConfigManager/ConfigManager.h"

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

class WeatherManager {
public:
  // 构造函数
  WeatherManager(const char* apiKey, const String& cityCode, BM8563* rtc, int eepromSize = 512);
  
  // 析构函数
  ~WeatherManager();
  
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
  
  // 从存储读取天气信息
  bool readWeatherFromStorage();
  
  // 将天气信息写入存储
  bool writeWeatherToStorage();
  
  // 设置更新间隔（秒）
  void setUpdateInterval(unsigned long intervalSeconds);
  
  // 获取上次更新时间
  unsigned long getLastUpdateTime();
  
  // 设置上次更新时间戳
  bool setUpdateTime(unsigned long timestamp);
  
  // 清除存储中的天气数据
  void clearWeatherData();
  
  // 将天气状况映射到符号
  static char mapWeatherToSymbol(const String& weather);
  
  // 将中文风向转换为英文
  static String translateWindDirection(const String& chineseDirection);
  
  // 格式化风速字符串
  static String formatWindSpeed(const String& windSpeed);
  
  // 获取天气信息字符串
  static String getWeatherInfo(const WeatherInfo& currentWeather);
  
  // 获取天气符号
  static char getWeatherSymbol(const WeatherInfo& currentWeather);

private:
  // API配置
  String _apiKey;
  String _cityCode;
  
  // 硬件引用
  BM8563* _rtc;
  
  // 配置管理器
  ConfigManager<ConfigData>* _configManager;
  
  // 更新间隔（默认30分钟）
  unsigned long _updateIntervalSeconds;
  
  // 当前天气信息
  WeatherInfo _currentWeather;
  
  // 私有方法
  void initializeDefaultWeather();
  void convertToConfigData(const WeatherInfo& weatherInfo, ConfigData& configData);
  void convertFromConfigData(const ConfigData& configData, WeatherInfo& weatherInfo);
  unsigned long getCurrentUnixTimestamp();
};

#endif // WEATHER_MANAGER_H