#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <time.h>
#include <Wire.h>
#include <ESP8266mDNS.h>
#include "../lib/BM8563/BM8563.h"
#include "../lib/GDEY029T94/GDEY029T94.h"
#include "../lib/WeatherManager/WeatherManager.h"
#include "../lib/WiFiManager/WiFiManager.h"
#include "../lib/Fonts/Weather_Symbols_Regular9pt7b.h"
#include "../lib/Fonts/DSEG7Modern_Bold28pt7b.h"

// 深度睡眠相关定义
#define DEEP_SLEEP_DURATION 60  // 1分钟深度睡眠（单位：秒）

// I2C引脚定义 (根据用户提供的连接)
#define SDA_PIN 2  // GPIO-2 (D4)
#define SCL_PIN 12 // GPIO-12 (D6)

// GDEY029T94 墨水屏引脚定义
#define EPD_CS    D8
#define EPD_DC    D2
#define EPD_RST   D0
#define EPD_BUSY  D1

DateTime currentTime = {0, 0, 0, 0, 0, 0};

// 创建BM8563对象实例
BM8563 rtc(SDA_PIN, SCL_PIN);

// 创建GDEY029T94对象实例
GDEY029T94 epd(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY);

// 创建WiFiManager对象实例
WiFiManager wifiManager;

// 高德地图API配置
const char* AMAP_API_KEY = "b4bed4011e9375d01423a45fba58e836";
String cityCode = "110108";  // 北京海淀区，可根据需要修改

// 创建WeatherManager对象实例
WeatherManager weatherManager(AMAP_API_KEY, cityCode, &rtc, 512);

// 函数声明
void updateNTPTime();

// BM8563 RTC 函数声明
bool readTimeFromBM8563();
bool writeTimeToBM8563(const DateTime& dt);

// 深度睡眠相关函数声明
void goToDeepSleep();

void setup() {
  Serial.begin(74880);
  
  Serial.println("System starting up...");
  
  // 初始化WeatherManager
  weatherManager.begin();
  
  // 初始化GDEY029T94墨水屏
  epd.begin();
  epd.setRotation(1); // 调整旋转以适应128x296分辨率
  epd.setTimeFont(&DSEG7Modern_Bold28pt7b);
  epd.setWeatherSymbolFont(&Weather_Symbols_Regular9pt7b);
  
  // 初始化BM8563 RTC
  if (rtc.begin()) {
    Serial.println("BM8563 RTC initialized successfully");
    
    // 清除RTC中断标志
    rtc.clearTimerFlag();
    rtc.clearAlarmFlag();
    Serial.println("RTC interrupt flags cleared");
    
    // 确保中断被禁用，防止INT持续拉低
    rtc.enableTimerInterrupt(false);
    rtc.enableAlarmInterrupt(false);
    Serial.println("RTC interrupts disabled");
  } else {
    Serial.println("Failed to initialize BM8563 RTC");
  }
  
  // 从RTC读取时间
  readTimeFromBM8563();
  
  // 判断是否需要从网络更新天气
  if (weatherManager.shouldUpdateFromNetwork()) {
    Serial.println("Weather data is outdated, updating from network...");
    
    // 初始化WiFi连接（使用默认配置）
    wifiManager.begin();
    
    // 如果WiFi连接成功，更新NTP时间和天气信息
    if (wifiManager.autoConnect()) {
      updateNTPTime();
      weatherManager.updateWeather(true);
    } else {
      Serial.println("WiFi connection failed, using cached data");
    }
  } else {
    Serial.println("Weather data is recent, using cached data");
  }
  
  // 获取当前天气信息并显示
  WeatherInfo currentWeather = weatherManager.getCurrentWeather();
  epd.showTimeDisplay(currentTime, currentWeather);
  
  // 进入深度睡眠
  goToDeepSleep();
}

void loop() {

}


// 更新NTP时间
void updateNTPTime() {
  if (!wifiManager.isConnected()) {
    Serial.println("WiFi not connected, skipping NTP update");
    return;
  }
  
  Serial.println("Updating time from NTP server...");
  
  // 配置NTP服务器和时区
  configTime(8 * 3600, 0, "ntp.aliyun.com", "ntp1.aliyun.com", "ntp2.aliyun.com");
  
  // 等待时间同步
  int retryCount = 0;
  const int maxRetries = 10;
  while (time(nullptr) < 1000000000 && retryCount < maxRetries) {
    delay(500);
    retryCount++;
    Serial.print(".");
  }
  
  if (retryCount >= maxRetries) {
    Serial.println("Failed to get time from NTP server");
    return;
  }
  
  // 获取时间
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  
  // 更新系统时间
  currentTime.year = (timeinfo->tm_year + 1900) % 100;  // 转换为两位数年份
  currentTime.month = timeinfo->tm_mon + 1;            // tm_mon是从0开始的
  currentTime.day = timeinfo->tm_mday;
  currentTime.hour = timeinfo->tm_hour;
  currentTime.minute = timeinfo->tm_min;
  currentTime.second = timeinfo->tm_sec;
  
  // 同步时间到BM8563 RTC
  writeTimeToBM8563(currentTime);
  
  Serial.print("Time updated from NTP: ");
  Serial.print(timeinfo->tm_year + 1900);
  Serial.print("/");
  Serial.print(timeinfo->tm_mon + 1);
  Serial.print("/");
  Serial.print(timeinfo->tm_mday);
  Serial.print(" ");
  Serial.print(timeinfo->tm_hour);
  Serial.print(":");
  Serial.print(timeinfo->tm_min);
  Serial.print(":");
  Serial.println(timeinfo->tm_sec);
}


// 从BM8563读取时间
bool readTimeFromBM8563() {
  BM8563_Time rtcTime;
  
  if (rtc.getTime(&rtcTime)) {
    // 更新currentTime
    currentTime.second = rtcTime.seconds;
    currentTime.minute = rtcTime.minutes;
    currentTime.hour = rtcTime.hours;
    currentTime.day = rtcTime.days;
    currentTime.month = rtcTime.months;
    currentTime.year = rtcTime.years;
    
    Serial.print("Time read from BM8563: ");
    Serial.print(2000 + currentTime.year);
    Serial.print("/");
    Serial.print(currentTime.month);
    Serial.print("/");
    Serial.print(currentTime.day);
    Serial.print(" ");
    Serial.print(currentTime.hour);
    Serial.print(":");
    Serial.print(currentTime.minute);
    Serial.print(":");
    Serial.println(currentTime.second);
    
    return true;
  } else {
    Serial.println("Failed to read time from BM8563");
    return false;
  }
}

// 写入时间到BM8563
bool writeTimeToBM8563(const DateTime& dt) {
  BM8563_Time rtcTime;
  
  // 转换DateTime到BM8563_Time
  rtcTime.seconds = dt.second;
  rtcTime.minutes = dt.minute;
  rtcTime.hours = dt.hour;
  rtcTime.days = dt.day;
  rtcTime.weekdays = 0; // 不使用
  rtcTime.months = dt.month;
  rtcTime.years = dt.year;
  
  if (rtc.setTime(&rtcTime)) {
    Serial.print("Time written to BM8563: ");
    Serial.print(2000 + dt.year);
    Serial.print("/");
    Serial.print(dt.month);
    Serial.print("/");
    Serial.print(dt.day);
    Serial.print(" ");
    Serial.print(dt.hour);
    Serial.print(":");
    Serial.print(dt.minute);
    Serial.print(":");
    Serial.println(dt.second);
    
    return true;
  } else {
    Serial.println("Failed to write time to BM8563");
    return false;
  }
}

// 设置并进入深度睡眠
void goToDeepSleep() {
  Serial.println("Setting up and entering deep sleep...");
  
  // 清除之前的定时器标志和闹钟标志
  rtc.clearTimerFlag();
  rtc.clearAlarmFlag();
  Serial.println("RTC interrupt flags cleared");
  
  // 设置RTC定时器，1分钟唤醒一次
  rtc.setTimer(60, BM8563_TIMER_1HZ);
  
  // 启用定时器中断
  rtc.enableTimerInterrupt(true);
  Serial.println("Timer interrupt enabled");
  
  Serial.println("Entering deep sleep...");
  Serial.flush();
  
  // 等待串口输出完成
  delay(100);
  
  // 进入深度睡眠，由RTC定时器唤醒
  ESP.deepSleep(0); // 0表示无限期睡眠，直到外部复位
}

