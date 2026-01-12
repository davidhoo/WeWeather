/**
 * @file main.cpp
 * @brief WeWeather - ESP8266 天气显示终端主程序
 * @details 基于ESP8266的低功耗天气显示系统，使用墨水屏显示天气信息
 *          核心功能：天气显示、时间管理、温湿度监测、电池监控、WiFi配网
 *          工作模式：深度睡眠 + RTC定时唤醒，实现超低功耗运行
 * @version 2.0
 * @date 2026-01-12
 */

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

// ==================== 全局对象实例 ====================

// RTC时钟模块 (BM8563) - 用于时间管理和深度睡眠唤醒
BM8563 rtc(I2C_SDA_PIN, I2C_SCL_PIN);

// 时间管理器 - 负责NTP同步和时间维护
TimeManager timeManager(&rtc);

// 墨水屏显示模块 (GDEY029T94 2.9寸) - 用于显示天气和时间信息
GDEY029T94 epd(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN);

// WiFi管理器 - 负责WiFi连接和配网
WiFiManager wifiManager;

// 天气管理器 - 负责天气数据获取和缓存
WeatherManager weatherManager(AMAP_API_KEY, CITY_CODE, &rtc, 512);

// 温湿度传感器 (SHT40) - 用于环境监测
SHT40 sht40(I2C_SDA_PIN, I2C_SCL_PIN);

// 电池监控模块 - 用于电量监测
BatteryMonitor battery;

// ==================== 函数声明 ====================
void goToDeepSleep();
void initializeSerial();
void initializeManagers();
void initializeSensors();
void initializeDisplay();
void initializeRTC();
void connectAndUpdateWiFi();
void updateAndDisplay();

// ==================== 初始化函数 ====================

/**
 * @brief 初始化串口通信
 * @note ESP8266 ROM bootloader 使用 74880 波特率，保持一致便于查看启动信息
 */
void initializeSerial() {
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.println("System starting up...");
}

/**
 * @brief 初始化管理器模块
 * @note 必须在硬件初始化之后调用
 */
void initializeManagers() {
  weatherManager.begin();
  timeManager.begin();
}

/**
 * @brief 初始化SHT40温湿度传感器
 * @note 传感器初始化失败不影响系统运行，将使用NAN值
 */
void initializeSensors() {
  if (sht40.begin()) {
    Serial.println("SHT40 initialized successfully");
  } else {
    Serial.println("Warning: SHT40 init failed, will use NAN values");
  }
}

/**
 * @brief 初始化GDEY029T94墨水屏
 * @note 旋转角度配置在config.h中定义
 */
void initializeDisplay() {
  epd.begin();
  epd.setRotation(DISPLAY_ROTATION); // 旋转角度以适应显示方向
  epd.setTimeFont(&DSEG7Modern_Bold28pt7b);
  epd.setWeatherSymbolFont(&Weather_Symbols_Regular9pt7b);
}

/**
 * @brief 初始化BM8563 RTC模块
 * @note 清除中断标志防止INT引脚持续拉低导致无法进入深度睡眠
 */
void initializeRTC() {
  if (rtc.begin()) {
    Serial.println("BM8563 RTC initialized successfully");
    
    // 清除RTC中断标志，防止INT引脚持续拉低
    rtc.clearTimerFlag();
    rtc.clearAlarmFlag();
    Serial.println("RTC interrupt flags cleared");
    
    // 确保中断被禁用
    rtc.enableTimerInterrupt(false);
    rtc.enableAlarmInterrupt(false);
    Serial.println("RTC interrupts disabled");
  } else {
    Serial.println("Error: Failed to initialize BM8563 RTC");
  }
}

/**
 * @brief 连接WiFi并更新网络数据
 * @note 仅在需要更新天气数据时才连接WiFi以节省电量
 */
void connectAndUpdateWiFi() {
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
}

/**
 * @brief 采集传感器数据并更新显示
 * @note 按顺序获取天气、时间、温湿度、电池状态，然后刷新显示
 */
void updateAndDisplay() {
  // 获取当前天气信息和时间
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
  
  // 更新墨水屏显示
  epd.showTimeDisplay(currentTime, currentWeather, temperature, humidity, batteryPercentage);
}

// ==================== 主程序 ====================

/**
 * @brief 系统初始化和主流程
 * @note 执行顺序：串口 -> 管理器 -> 传感器 -> 显示 -> RTC -> WiFi -> 数据采集 -> 睡眠
 */
void setup() {
  initializeSerial();
  initializeManagers();
  initializeSensors();
  initializeDisplay();
  initializeRTC();
  
  connectAndUpdateWiFi();
  updateAndDisplay();
  
  goToDeepSleep();
}

/**
 * @brief 主循环函数
 * @note 本项目使用深度睡眠模式，不使用loop()函数
 */
void loop() {
  // 空函数 - 系统在setup()结束后进入深度睡眠，不会执行到这里
}

// ==================== 电源管理 ====================

/**
 * @brief 配置RTC定时器并进入深度睡眠
 * @note 深度睡眠流程：
 *       1. 清除RTC中断标志，防止立即唤醒
 *       2. 配置RTC定时器（使用config.h中的RTC_TIMER_SECONDS）
 *       3. 启用定时器中断
 *       4. 进入ESP8266深度睡眠模式
 *       5. RTC定时器到期后通过INT引脚触发硬件复位唤醒
 */
void goToDeepSleep() {
  Serial.println("Setting up and entering deep sleep...");
  
  // 清除之前的定时器标志和闹钟标志，防止INT引脚持续拉低
  rtc.clearTimerFlag();
  rtc.clearAlarmFlag();
  Serial.println("RTC interrupt flags cleared");
  
  // 设置RTC定时器，使用配置文件中的时间（默认60秒）
  rtc.setTimer(RTC_TIMER_SECONDS, BM8563_TIMER_1HZ);
  
  // 启用定时器中断，定时器到期时INT引脚拉低触发唤醒
  rtc.enableTimerInterrupt(true);
  Serial.println("Timer interrupt enabled");
  
  Serial.println("Entering deep sleep...");
  Serial.flush(); // 确保串口数据发送完成
  
  // 短暂延时确保串口输出完成
  delay(100);
  
  // 进入深度睡眠模式
  // 参数0表示无限期睡眠，实际由RTC定时器通过INT引脚触发硬件复位唤醒
  // 唤醒后系统重启，从setup()重新开始执行
  ESP.deepSleep(0);
}

