#ifndef GDEY029T94_H
#define GDEY029T94_H

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold9pt7b.h>

// 日期时间结构体
struct DateTime {
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;
};

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

class GDEY029T94 {
public:
  // 构造函数
  GDEY029T94(uint8_t cs, uint8_t dc, uint8_t rst, uint8_t busy);
  
  // 初始化显示
  void begin();
  
  // 设置旋转方向
  void setRotation(int rotation);
  
  // 显示时间和天气信息
  void showTimeDisplay(const DateTime& currentTime, const WeatherInfo& currentWeather);
  
  // 设置时间字体
  void setTimeFont(const GFXfont* font);
  
  // 设置天气符号字体
  void setWeatherSymbolFont(const GFXfont* font);
  
  // 8像素对齐辅助函数
  int alignToPixel8(int x);
  
private:
  GxEPD2_BW<GxEPD2_290_GDEY029T94, GxEPD2_290_GDEY029T94::WIDTH> display;
  const GFXfont* timeFont;
  const GFXfont* weatherSymbolFont;
  
  // 获取格式化时间字符串
  String getFormattedTime(const DateTime& currentTime);
  
  // 获取格式化日期字符串
  String getFormattedDate(const DateTime& currentTime);
  
  // 获取天气信息字符串
  String getWeatherInfo(const WeatherInfo& currentWeather);
  
  // 获取天气符号
  char getWeatherSymbol(const WeatherInfo& currentWeather);
  
  // 获取星期几
  String getDayOfWeek(int year, int month, int day);
  
  // 将中文风向转换为英文
  String translateWindDirection(const String& chineseDirection);
  
  // 格式化风速字符串
  String formatWindSpeed(const String& windSpeed);
};

#endif // GDEY029T94_H