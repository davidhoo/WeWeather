/**
 * @file main.cpp
 * @brief WeWeather - ESP8266 天气显示终端主程序 (支持AP+Web配置)
 * @details 基于ESP8266的低功耗天气显示系统，使用墨水屏显示天气信息
 *          核心功能：天气显示、时间管理、温湿度监测、电池监控、WiFi配网、AP+Web配置
 *          工作模式：深度睡眠 + RTC定时唤醒，实现超低功耗运行
 *          配置模式：RXD引脚拉低时进入AP+Web配置模式
 * @version 2.1
 * @date 2026-01-12
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <time.h>
#include <Wire.h>
#include <ESP8266mDNS.h>
#include "../config.h"
#include "../lib/Logger/Logger.h"
#include "../lib/BM8563/BM8563.h"
#include "../lib/GDEY029T94/GDEY029T94.h"
#include "../lib/WeatherManager/WeatherManager.h"
#include "../lib/WiFiManager/WiFiManager.h"
#include "../lib/TimeManager/TimeManager.h"
#include "../lib/SHT40/SHT40.h"
#include "../lib/BatteryMonitor/BatteryMonitor.h"
#include "../lib/ConfigManager/ConfigManager.h"
#include "../lib/WebConfig/WebConfig.h"
#include "../lib/Fonts/Weather_Symbols_Regular9pt7b.h"
#include "../lib/Fonts/DSEG7Modern_Bold28pt7b.h"

// ==================== 全局对象实例 ====================

// 配置管理器 - 负责EEPROM配置存储
ConfigManager configManager;

// Web配置管理器 - 负责AP+Web配置模式
WebConfig webConfig(&configManager);

// RTC时钟模块 (BM8563) - 用于时间管理和深度睡眠唤醒
BM8563 rtc(I2C_SDA_PIN, I2C_SCL_PIN);

// 时间管理器 - 负责NTP同步和时间维护
TimeManager timeManager(&rtc);

// 墨水屏显示模块 (GDEY029T94 2.9寸) - 用于显示天气和时间信息
GDEY029T94 epd(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN);

// WiFi管理器 - 负责WiFi连接和配网
WiFiManager wifiManager;

// 天气管理器 - 将在setup中根据配置初始化
WeatherManager* weatherManager = nullptr;

// 温湿度传感器 (SHT40) - 用于环境监测
SHT40 sht40(I2C_SDA_PIN, I2C_SCL_PIN);

// 电池监控模块 - 用于电量监测
BatteryMonitor battery;

// 全局配置变量
DeviceConfig deviceConfig;
bool configLoaded = false;

// ==================== 函数声明 ====================
bool checkConfigMode();
void enterConfigMode();
void displayConfigInfo(const String& ssid, const String& ip);
void goToDeepSleep();
void initializeSerial();
void initializeConfigManager();
void initializeWeatherManager();
void initializeSensors();
void initializeDisplay();
void initializeRTC();
void initializeTimeManager();
void connectAndUpdateWiFi();
void updateAndDisplay();
void applyMacAddress();

// ==================== 配置模式相关函数 ====================

/**
 * @brief 检查是否需要进入配置模式
 * @return true 如果RXD引脚被拉低，需要进入配置模式
 */
bool checkConfigMode() {
  pinMode(RXD_PIN, INPUT_PULLUP);
  delay(50); // 稳定读取
  bool configMode = (digitalRead(RXD_PIN) == LOW);
  
  if (configMode) {
    Logger::info(F("Config"), F("RXD pin LOW - entering config mode"));
  } else {
    Logger::info(F("Config"), F("RXD pin HIGH - normal mode"));
  }
  
  return configMode;
}

/**
 * @brief 进入AP+Web配置模式
 */
void enterConfigMode() {
  Logger::info(F("Config"), F("Entering AP+Web config mode..."));
  
  // 清除RTC唤醒定时器，避免配置过程中被重置
  rtc.resetInterrupts();
  rtc.clearTimer();
  Logger::info(F("RTC"), F("Wakeup timer cleared for config mode"));
  
  // 显示配置信息到屏幕
  displayConfigInfo("weweather", "192.168.4.1");
  
  // 初始化并启动Web配置
  webConfig.begin("weweather", "");
  
  // 进入配置模式（阻塞式，等待配置完成或超时）
  bool configured = webConfig.enterConfigMode(300000); // 5分钟超时
  
  if (configured) {
    Logger::info(F("Config"), F("Configuration completed successfully"));
    
    // 停止Web服务器
    webConfig.stop();
    
    // 显示配置完成信息
    epd.showConfigDisplay("weweather", "192.168.4.1", "Config Saved! Restarting...");
    
    delay(2000);
    
    // 重启设备应用新配置
    Logger::info(F("System"), F("Restarting to apply new config..."));
    ESP.restart();
  } else {
    Logger::warning(F("Config"), F("Configuration timeout or failed"));
    webConfig.stop();
    
    // 显示超时信息
    epd.showConfigDisplay("weweather", "192.168.4.1", "Config Timeout! Continuing...");
    
    delay(2000);
  }
}

/**
 * @brief 在屏幕上显示配置信息
 * @param ssid AP热点名称
 * @param ip AP IP地址
 */
void displayConfigInfo(const String& ssid, const String& ip) {
  epd.showConfigDisplay(ssid, ip);
  Logger::info(F("Display"), F("Config info displayed"));
}

// ==================== 初始化函数 ====================

/**
 * @brief 初始化串口通信
 * @note ESP8266 ROM bootloader 使用 74880 波特率，保持一致便于查看启动信息
 */
void initializeSerial() {
  Logger::begin(SERIAL_BAUD_RATE);
  Logger::info(F("System"), F("Starting up..."));
}

/**
 * @brief 初始化配置管理器
 */
void initializeConfigManager() {
  configManager.begin();
  
  // 尝试从EEPROM加载配置
  if (configManager.loadConfig(deviceConfig)) {
    Logger::info(F("Config"), F("Loaded config from EEPROM"));
    configLoaded = true;
    
    // 应用MAC地址配置
    applyMacAddress();
  } else {
    Logger::warning(F("Config"), F("No valid config found, using defaults"));
    configLoaded = false;
    
    // 使用默认配置
    memset(&deviceConfig, 0, sizeof(DeviceConfig));
    strncpy(deviceConfig.ssid, DEFAULT_WIFI_SSID, sizeof(deviceConfig.ssid) - 1);
    strncpy(deviceConfig.password, DEFAULT_WIFI_PASSWORD, sizeof(deviceConfig.password) - 1);
    strncpy(deviceConfig.amapApiKey, DEFAULT_AMAP_API_KEY, sizeof(deviceConfig.amapApiKey) - 1);
    strncpy(deviceConfig.cityCode, DEFAULT_CITY_CODE, sizeof(deviceConfig.cityCode) - 1);
    strncpy(deviceConfig.macAddress, DEFAULT_MAC_ADDRESS, sizeof(deviceConfig.macAddress) - 1);
    
    // 应用默认MAC地址
    applyMacAddress();
  }
}

/**
 * @brief 应用MAC地址配置
 */
void applyMacAddress() {
  if (strlen(deviceConfig.macAddress) > 0) {
    // 解析MAC地址字符串
    uint8_t mac[6];
    unsigned int m[6];
    if (sscanf(deviceConfig.macAddress, "%02x:%02x:%02x:%02x:%02x:%02x",
               &m[0], &m[1], &m[2], &m[3], &m[4], &m[5]) == 6) {
      // 转换为uint8_t
      for (int i = 0; i < 6; i++) {
        mac[i] = (uint8_t)m[i];
      }
      
      // 设置自定义MAC地址
      wifi_set_macaddr(STATION_IF, mac);
      wifi_set_macaddr(SOFTAP_IF, mac);
      
      Logger::info(F("WiFi"), F("Custom MAC address applied"));
      Serial.print(F("MAC: "));
      Serial.println(deviceConfig.macAddress);
    } else {
      Logger::warning(F("WiFi"), F("Invalid MAC address format, using default"));
    }
  } else {
    Logger::info(F("WiFi"), F("Using hardware default MAC address"));
  }
}

/**
 * @brief 初始化WeatherManager
 * @note 使用配置中的API Key和城市代码
 */
void initializeWeatherManager() {
  if (weatherManager) {
    delete weatherManager;
  }
  
  weatherManager = new WeatherManager(deviceConfig.amapApiKey, deviceConfig.cityCode, &rtc, 512);
  weatherManager->begin();
  
  Logger::info(F("Weather"), F("WeatherManager initialized"));
}

/**
 * @brief 初始化SHT40温湿度传感器
 * @note 传感器初始化失败不影响系统运行，将使用NAN值
 */
void initializeSensors() {
  if (sht40.begin()) {
    Logger::info(F("SHT40"), F("Initialized successfully"));
  } else {
    Logger::warning(F("SHT40"), F("Init failed, will use NAN values"));
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
    Logger::info(F("RTC"), F("BM8563 initialized successfully"));
    
    // 重置所有中断标志和禁用中断，防止INT引脚持续拉低
    rtc.resetInterrupts();
    Logger::info(F("RTC"), F("Interrupts reset"));
  } else {
    Logger::error(F("RTC"), F("Failed to initialize BM8563"));
  }
}

/**
 * @brief 初始化TimeManager
 * @note 必须在RTC初始化之后调用，用于从RTC读取时间
 */
void initializeTimeManager() {
  timeManager.begin();
}

/**
 * @brief 连接WiFi并更新网络数据
 * @note 使用配置中的WiFi信息连接
 */
void connectAndUpdateWiFi() {
  if (weatherManager->shouldUpdateFromNetwork()) {
    Logger::info(F("Weather"), F("Data is outdated, updating from network..."));
    
    // 使用配置中的WiFi信息连接
    wifiManager.begin();
    wifiManager.setCredentials(deviceConfig.ssid, deviceConfig.password);
    
    // 如果WiFi连接成功，更新NTP时间和天气信息
    if (wifiManager.autoConnect()) {
      timeManager.setWiFiConnected(true);
      timeManager.updateNTPTime();
      weatherManager->updateWeather(true);
    } else {
      Logger::warning(F("WiFi"), F("Connection failed, using cached data"));
      timeManager.setWiFiConnected(false);
    }
  } else {
    Logger::info(F("Weather"), F("Data is recent, using cached data"));
  }
}

/**
 * @brief 采集传感器数据并更新显示
 * @note 按顺序获取天气、时间、温湿度、电池状态，然后刷新显示
 */
void updateAndDisplay() {
  // 获取当前天气信息和时间
  WeatherInfo currentWeather = weatherManager->getCurrentWeather();
  DateTime currentTime = timeManager.getCurrentTime();
  
  // 读取温湿度数据（一次性读取，避免重复测量）
  float temperature, humidity;
  if (sht40.readTemperatureHumidity(temperature, humidity)) {
    // 使用 infoValue 方法输出带数值的日志，避免 String 拼接占用 RAM
    Logger::infoValue(F("SHT40"), F("Temperature:"), temperature, F("°C"), 1);
    Logger::infoValue(F("SHT40"), F("Humidity:"), humidity, F("%RH"), 1);
  } else {
    Logger::warning(F("SHT40"), F("Failed to read sensor"));
    temperature = NAN;
    humidity = NAN;
  }
  
  // 初始化并读取电池状态
  battery.begin();
  int rawADC = battery.getRawADC();
  float batteryVoltage = battery.getBatteryVoltage();
  float batteryPercentage = battery.getBatteryPercentage();
  
  // 打印电池状态信息
  Logger::infoValue(F("Battery"), F("Raw ADC:"), rawADC);
  Logger::infoValue(F("Battery"), F("Voltage:"), batteryVoltage, F("V"), 2);
  Logger::infoValue(F("Battery"), F("Percentage:"), batteryPercentage, F("%"), 1);
  
  // 更新墨水屏显示
  epd.showTimeDisplay(currentTime, currentWeather, temperature, humidity, batteryPercentage);
}

// ==================== 主程序 ====================

/**
 * @brief 系统初始化和主流程
 * @note 执行顺序：串口 -> 配置检查 -> 配置模式或正常模式
 */
void setup() {
  initializeSerial();
  
  // 检查是否需要进入配置模式
  if (checkConfigMode()) {
    // 进入配置模式
    initializeDisplay();
    initializeRTC();
    initializeConfigManager();
    enterConfigMode();
    // 配置模式结束后会重启或继续正常流程
  }
  
  // 正常模式流程
  initializeConfigManager();
  initializeWeatherManager();
  initializeSensors();
  initializeDisplay();
  initializeRTC();
  initializeTimeManager();
  
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
 *       1. 使用setupWakeupTimer()配置RTC定时器（自动清除中断标志并启用定时器中断）
 *       2. 进入ESP8266深度睡眠模式
 *       3. RTC定时器到期后通过INT引脚触发硬件复位唤醒
 *       4. 唤醒后系统重启，从setup()重新开始执行
 */
void goToDeepSleep() {
  Logger::info(F("Power"), F("Setting up and entering deep sleep..."));
  
  // 配置RTC唤醒定时器（使用config.h中的RTC_TIMER_SECONDS，默认60秒）
  // 此方法会自动清除中断标志、设置定时器并启用中断
  rtc.setupWakeupTimer(RTC_TIMER_SECONDS);
  Logger::info(F("RTC"), F("Wakeup timer configured"));
  
  Logger::info(F("Power"), F("Entering deep sleep..."));
  Serial.flush(); // 确保串口数据发送完成
  
  // 短暂延时确保串口输出完成
  delay(100);
  
  // 进入深度睡眠模式
  // 参数0表示无限期睡眠，实际由RTC定时器通过INT引脚触发硬件复位唤醒
  ESP.deepSleep(0);
}
