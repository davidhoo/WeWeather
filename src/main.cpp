#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <time.h>
#include <Wire.h>
#include <ESP8266mDNS.h>
#include "../config.h"
#include "../lib/LogManager/LogManager.h"
#include "../lib/BM8563/BM8563.h"
#include "../lib/GDEY029T94/GDEY029T94.h"
#include "../lib/WeatherManager/WeatherManager.h"
#include "../lib/WiFiManager/WiFiManager.h"
#include "../lib/TimeManager/TimeManager.h"
#include "../lib/SHT40/SHT40.h"
#include "../lib/BatteryMonitor/BatteryMonitor.h"
#include "../lib/Fonts/Weather_Symbols_Regular9pt7b.h"
#include "../lib/Fonts/DSEG7Modern_Bold28pt7b.h"

// 创建BM8563对象实例
BM8563 rtc(I2C_SDA_PIN, I2C_SCL_PIN);

// 创建TimeManager对象实例
TimeManager timeManager(&rtc);

// 创建GDEY029T94对象实例
GDEY029T94 epd(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN);

// 创建WiFiManager对象实例
WiFiManager wifiManager;

// 创建WeatherManager对象实例
WeatherManager weatherManager(DEFAULT_AMAP_API_KEY, DEFAULT_CITY_CODE, &rtc, 512);

// 创建SHT40对象实例
SHT40 sht40(I2C_SDA_PIN, I2C_SCL_PIN);

// 创建BatteryMonitor对象实例
BatteryMonitor battery;

// 深度睡眠相关函数声明
void goToDeepSleep();
void setup() {
  // 初始化日志管理器
  LogManager::begin(SERIAL_BAUD_RATE, LOG_INFO);
  
  LOG_INFO("System starting up...");
  
  // 初始化WeatherManager
  weatherManager.begin();
  
  // 初始化SHT40温湿度传感器
  if (sht40.begin()) {
    LOG_INFO("SHT40 initialized successfully");
  } else {
    LOG_ERROR("Failed to initialize SHT40");
  }
  
  // 初始化GDEY029T94墨水屏
  epd.begin();
  epd.setRotation(DISPLAY_ROTATION);
  epd.setTimeFont(&DSEG7Modern_Bold28pt7b);
  epd.setWeatherSymbolFont(&Weather_Symbols_Regular9pt7b);
  
  // 初始化BM8563 RTC
  if (rtc.begin()) {
    LOG_INFO("BM8563 RTC initialized successfully");
    
    // 清除RTC中断标志
    rtc.clearTimerFlag();
    rtc.clearAlarmFlag();
    LOG_INFO("RTC interrupt flags cleared");
    
    // 确保中断被禁用，防止INT持续拉低
    rtc.enableTimerInterrupt(false);
    rtc.enableAlarmInterrupt(false);
    LOG_INFO("RTC interrupts disabled");
  } else {
    LOG_ERROR("Failed to initialize BM8563 RTC");
  }
  
  // 初始化TimeManager并从RTC读取时间
  timeManager.begin();
  
  // 判断是否需要从网络更新天气
  if (weatherManager.shouldUpdateFromNetwork()) {
    LOG_INFO("Weather data is outdated, updating from network...");
    
    // 初始化WiFi连接（使用默认配置）
    wifiManager.begin();
    
    // 如果WiFi连接成功，更新NTP时间和天气信息
    if (wifiManager.autoConnect()) {
      timeManager.setWiFiConnected(true);
      timeManager.updateNTPTime();
      weatherManager.updateWeather(true);
    } else {
      LOG_WARN("WiFi connection failed, using cached data");
      timeManager.setWiFiConnected(false);
    }
  } else {
    LOG_INFO("Weather data is recent, using cached data");
  }
  
  // 获取当前天气信息并显示
  WeatherInfo currentWeather = weatherManager.getCurrentWeather();
  DateTime currentTime = timeManager.getCurrentTime();
  
  // 读取温湿度数据（一次性读取，避免重复测量）
  float temperature, humidity;
  if (sht40.readTemperatureHumidity(temperature, humidity)) {
    LOG_INFO_F("Current Temperature: %.1f °C", temperature);
    LOG_INFO_F("Current Humidity: %.1f %%RH", humidity);
  } else {
    LOG_ERROR("Failed to read SHT40 sensor");
    temperature = NAN;
    humidity = NAN;
  }
  
  
  // 初始化并读取电池状态
  battery.begin();
  int rawADC = battery.getRawADC();
  float batteryVoltage = battery.getBatteryVoltage();
  float batteryPercentage = battery.getBatteryPercentage();
  
  // 打印电池状态信息
  LogManager::printSeparator('=', 15);
  LogManager::info(F("电池状态"));
  LogManager::printSeparator('=', 15);
  LogManager::printKeyValue(F("原始 ADC 值"), rawADC);
  LogManager::printKeyValue(F("电池电压"), batteryVoltage, 2);
  LogManager::printKeyValue(F("电池电量"), batteryPercentage, 1);
  LogManager::printSeparator('=', 15);
  

  epd.showTimeDisplay(currentTime, currentWeather, temperature, humidity, batteryPercentage);
  // 进入深度睡眠
  goToDeepSleep();
}

void loop() {

}



// 设置并进入深度睡眠
void goToDeepSleep() {
  LOG_INFO("Setting up and entering deep sleep...");
  
  // 清除之前的定时器标志和闹钟标志
  rtc.clearTimerFlag();
  rtc.clearAlarmFlag();
  LOG_INFO("RTC interrupt flags cleared");
  
  // 设置RTC定时器，使用配置中的时间
  rtc.setTimer(RTC_TIMER_SECONDS, BM8563_TIMER_1HZ);
  
  // 启用定时器中断
  rtc.enableTimerInterrupt(true);
  LOG_INFO("Timer interrupt enabled");
  
  LOG_INFO("Entering deep sleep...");
  Serial.flush();
  
  // 等待串口输出完成
  delay(100);
  
  // 进入深度睡眠，由RTC定时器唤醒
  ESP.deepSleep(0); // 0表示无限期睡眠，直到外部复位
}

