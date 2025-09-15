#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "SevenSegment42pt7b.h"
#include "Weather_Symbols_Regular9pt7b.h"

// GDEY029T94 2.9寸黑白墨水屏 128x296 (使用SSD1680控制器)
struct DateTime {
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;
};

// Weather information structure
struct WeatherInfo {
  float temperature;    // Temperature in Celsius
  int humidity;         // Humidity percentage
  char symbol;          // Weather symbol character
  // Weather symbol mapping:
  // n=晴(sunny), d=雪(snow), m=雨(rain), l=雾(fog), c=阴(overcast), o=多云(cloudy), k=雷雨(thunderstorm)
};

DateTime currentTime = {25, 9, 10, 20, 1, 34};
WeatherInfo currentWeather = {23.5, 65, 'n'}; // 23.5°C, 65% humidity, sunny
unsigned long lastMillis = 0;
unsigned long lastFullRefresh = 0;    // 上次全屏刷新时间
unsigned long lastPartialRefresh = 0; // 上次局部刷新时间
int lastDisplayedMinute = -1;         // 上次显示的分钟数，用于检测分钟变化
bool isFirstPartialRefresh = true;    // 标记是否为第一次局部刷新

#define EPD_CS    D8
#define EPD_DC    D2
#define EPD_RST   D4
#define EPD_BUSY  D1
// 原始定义可能存在width/height混淆，尝试使用WIDTH而不是HEIGHT
GxEPD2_BW<GxEPD2_290_GDEY029T94, GxEPD2_290_GDEY029T94::WIDTH> display(GxEPD2_290_GDEY029T94(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

// 基础函数声明
void showTimeDisplay();
void showPartialTimeDisplay(); // 局部刷新时钟区域
void updateTime();
String getFormattedTime();
String getFormattedDate();
String getWeatherInfo();
char getWeatherSymbol();
String getDayOfWeek(int year, int month, int day);
void processSerialInput();
void setTimeFromTimestamp(unsigned long timestamp);
DateTime timestampToDateTime(unsigned long timestamp);

void setup() {
  Serial.begin(115200);
  Serial.println("WeWeather - Partial Refresh Test");
  
  // 初始化显示屏
  display.init(115200, true, 2, false);
  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);
  
  // 显示启动信息
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(10, 30);
    display.println("WeWeather");
    display.setCursor(10, 60);
    display.println("Partial Refresh Test");
  } while(display.nextPage());
}

// 更新时间函数
void updateTime() {
  // 简单的时间递增模拟
  currentTime.second++;
  if (currentTime.second >= 60) {
    currentTime.second = 0;
    currentTime.minute++;
    if (currentTime.minute >= 60) {
      currentTime.minute = 0;
      currentTime.hour++;
      if (currentTime.hour >= 24) {
        currentTime.hour = 0;
        // 这里可以添加日期递增逻辑
      }
    }
  }
}

// 获取格式化时间字符串
String getFormattedTime() {
  char buffer[6];
  sprintf(buffer, "%02d:%02d", currentTime.hour, currentTime.minute);
  return String(buffer);
}

// 局部刷新时钟区域
void showPartialTimeDisplay() {
  // 这里将实现局部刷新逻辑
  Serial.println("Partial refresh: " + getFormattedTime());
  
  // 局部刷新实现
  // 定义需要刷新的区域（例如只刷新时间显示区域）
  uint16_t x = 10;
  uint16_t y = 30;
  uint16_t w = 100;
  uint16_t h = 30;
  
  // 使用局部刷新模式
  display.setPartialWindow(x, y, w, h);
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(x, y + 20);
    display.println("Time: " + getFormattedTime());
  } while(display.nextPage());
}

// 显示完整时间界面
void showTimeDisplay() {
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(10, 30);
    display.println("Current Time:");
    display.setCursor(20, 60);
    display.println(getFormattedTime());
  } while(display.nextPage());
}

// 获取格式化日期字符串
String getFormattedDate() {
  char buffer[11];
  sprintf(buffer, "%02d/%02d/%02d", currentTime.year, currentTime.month, currentTime.day);
  return String(buffer);
}

// 获取天气信息字符串
String getWeatherInfo() {
  char buffer[20];
  sprintf(buffer, "%.1fC %d%%", currentWeather.temperature, currentWeather.humidity);
  return String(buffer);
}

// 获取天气符号
char getWeatherSymbol() {
  return currentWeather.symbol;
}

// 获取星期几
String getDayOfWeek(int year, int month, int day) {
  // 简单实现，实际应用中可能需要更复杂的算法
  return "Mon";
}

// 处理串口输入
void processSerialInput() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    Serial.println("Received: " + input);
  }
}

// 从时间戳设置时间
void setTimeFromTimestamp(unsigned long timestamp) {
  // 简单实现
  currentTime.second = timestamp % 60;
  currentTime.minute = (timestamp / 60) % 60;
  currentTime.hour = (timestamp / 3600) % 24;
}

// 时间戳转换为日期时间结构
DateTime timestampToDateTime(unsigned long timestamp) {
  DateTime dt;
  dt.second = timestamp % 60;
  dt.minute = (timestamp / 60) % 60;
  dt.hour = (timestamp / 3600) % 24;
  // 简化实现，实际应用中需要更复杂的日期计算
  dt.day = 1;
  dt.month = 1;
  dt.year = 2025;
  return dt;
}

void loop() {
  // 每秒更新一次时间显示
  if (millis() - lastMillis >= 1000) {
    lastMillis = millis();
    updateTime();
    
    // 每分钟进行一次局部刷新
    if (currentTime.minute != lastDisplayedMinute) {
      showPartialTimeDisplay();
      lastDisplayedMinute = currentTime.minute;
    }
  }
  
  // 处理串口输入
  processSerialInput();
}