#ifndef GDEY029T94_H
#define GDEY029T94_H

#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "../TimeManager/TimeManager.h"
#include "../Logger/Logger.h"

// 前向声明 WeatherInfo 结构体（在 WeatherManager.h 中定义）
struct WeatherInfo;

class GDEY029T94 {
public:
  // 构造函数
  GDEY029T94(uint8_t cs, uint8_t dc, uint8_t rst, uint8_t busy);
  
  // 初始化显示
  void begin();
  
  // 设置旋转方向
  void setRotation(int rotation);
  
  // 显示时间和天气信息
  void showTimeDisplay(const DateTime& currentTime, const WeatherInfo& currentWeather, float temperature = NAN, float humidity = NAN, float batteryPercentage = NAN);
  
  // 设置时间字体
  void setTimeFont(const GFXfont* font);
  
  // 设置天气符号字体
  void setWeatherSymbolFont(const GFXfont* font);
  
  // 8像素对齐辅助函数
  int alignToPixel8(int x);
  
private:
  // 绘制电池符号
  void drawBatteryIcon(int x, int y, float percentage);
  GxEPD2_BW<GxEPD2_290_GDEY029T94, GxEPD2_290_GDEY029T94::HEIGHT> display;
  const GFXfont* timeFont;
  const GFXfont* weatherSymbolFont;
  
};

#endif // GDEY029T94_H