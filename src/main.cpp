#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <time.h>
#include <Wire.h>
#include <ESP8266mDNS.h>
#include "../config.h"
#include "../lib/BM8563/BM8563.h"
#include "../lib/GDEY029T94/GDEY029T94.h"
#include "../lib/WeatherManager/WeatherManager.h"
#include "../lib/WiFiManager/WiFiManager.h"
#include "../lib/TimeManager/TimeManager.h"
#include "../lib/SHT40/SHT40.h"
#include "../lib/BatteryMonitor/BatteryMonitor.h"
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

// 创建BM8563对象实例
BM8563 rtc(SDA_PIN, SCL_PIN);

// 创建TimeManager对象实例
TimeManager timeManager(&rtc);

// 创建GDEY029T94对象实例
GDEY029T94 epd(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY);

// 创建WiFiManager对象实例
WiFiManager wifiManager;

// 高德地图API配置（从config.h读取）
const char* amapApiKey = AMAP_API_KEY;
String cityCode = CITY_CODE;

// 创建WeatherManager对象实例
WeatherManager weatherManager(amapApiKey, cityCode, &rtc, 512);

// 创建SHT40对象实例
SHT40 sht40(SDA_PIN, SCL_PIN);

// 创建BatteryMonitor对象实例
BatteryMonitor battery;

// 深度睡眠相关函数声明
void goToDeepSleep();

void setup() {
  Serial.begin(74880);
  
  Serial.println("System starting up...");
  
  // 初始化WeatherManager
  weatherManager.begin();
  
  // 初始化SHT40温湿度传感器
  if (sht40.begin()) {
    Serial.println("SHT40 initialized successfully");
  } else {
    Serial.println("Failed to initialize SHT40");
  }
  
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
  
  // 初始化TimeManager并从RTC读取时间
  timeManager.begin();
  
  // 判断是否需要从网络更新天气
  if (weatherManager.shouldUpdateFromNetwork()) {
    Serial.println("Weather data is outdated, updating from network...");
    
    // 初始化WiFi连接（使用默认配置）
    wifiManager.begin();
    
    // 如果WiFi连接成功，更新NTP时间和天气信息
    if (wifiManager.autoConnect()) {
      timeManager.setWiFiConnected(true);
      timeManager.updateNTPTime();
      weatherManager.updateWeather(true);
    } else {
      Serial.println("WiFi connection failed, using cached data");
      timeManager.setWiFiConnected(false);
    }
  } else {
    Serial.println("Weather data is recent, using cached data");
  }
  
  // 获取当前天气信息并显示
  WeatherInfo currentWeather = weatherManager.getCurrentWeather();
  DateTime currentTime = timeManager.getCurrentTime();
  
  // 读取温湿度数据（一次性读取，避免重复测量）
  float temperature, humidity;
  if (sht40.readTemperatureHumidity(temperature, humidity)) {
    Serial.println("Current Temperature: " + String(temperature) + " °C");
    Serial.println("Current Humidity: " + String(humidity) + " %RH");
  } else {
    Serial.println("Failed to read SHT40 sensor");
    temperature = NAN;
    humidity = NAN;
  }
  
  
  // 初始化并读取电池状态
  battery.begin();
  int rawADC = battery.getRawADC();
  float batteryVoltage = battery.getBatteryVoltage();
  float batteryPercentage = battery.getBatteryPercentage();
  
  // 打印电池状态信息
  Serial.println("=== 电池状态 ===");
  Serial.println("原始 ADC 值: " + String(rawADC));
  Serial.println("电池电压: " + String(batteryVoltage, 2) + " V");
  Serial.println("电池电量: " + String(batteryPercentage, 1) + " %");
  Serial.println("================");
  

  epd.showTimeDisplay(currentTime, currentWeather, temperature, humidity, batteryPercentage);
  // 进入深度睡眠
  goToDeepSleep();
}

void loop() {

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

