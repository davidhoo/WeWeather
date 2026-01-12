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

// 创建BM8563对象实例（使用config.h中的I2C引脚配置）
BM8563 rtc(I2C_SDA_PIN, I2C_SCL_PIN);

// 创建TimeManager对象实例
TimeManager timeManager(&rtc);

// 创建GDEY029T94对象实例（使用config.h中的墨水屏引脚配置）
GDEY029T94 epd(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN);

// 创建WiFiManager对象实例
WiFiManager wifiManager;

// 高德地图API配置（从config.h读取）
const char* amapApiKey = AMAP_API_KEY;
String cityCode = CITY_CODE;

// 创建WeatherManager对象实例
WeatherManager weatherManager(amapApiKey, cityCode, &rtc, 512);

// 创建SHT40对象实例（使用config.h中的I2C引脚配置）
SHT40 sht40(I2C_SDA_PIN, I2C_SCL_PIN);

// 创建BatteryMonitor对象实例
BatteryMonitor battery;

// 函数声明
void initializeHardware();
void initializeRTC();
void initializeDisplay();
void initializeSensors();
bool handleWiFiConnection();
void handleConfigMode();
void updateWeatherData();
void readSensorData(float& temperature, float& humidity);
void readBatteryStatus(float& batteryPercentage);
void updateDisplay();
void goToDeepSleep();

/**
 * @brief 初始化硬件设备
 */
void initializeHardware() {
  Serial.begin(SERIAL_BAUD_RATE);
  Serial.println("System starting up...");
  
  // 初始化各个管理器
  weatherManager.begin();
  timeManager.begin();
}

/**
 * @brief 初始化RTC时钟模块
 */
void initializeRTC() {
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
}

/**
 * @brief 初始化墨水屏显示
 */
void initializeDisplay() {
  epd.begin();
  epd.setRotation(DISPLAY_ROTATION);
  epd.setTimeFont(&DSEG7Modern_Bold28pt7b);
  epd.setWeatherSymbolFont(&Weather_Symbols_Regular9pt7b);
  Serial.println("Display initialized successfully");
}

/**
 * @brief 初始化传感器
 */
void initializeSensors() {
  // 初始化SHT40温湿度传感器
  if (sht40.begin()) {
    Serial.println("SHT40 initialized successfully");
  } else {
    Serial.println("Failed to initialize SHT40");
  }
  
  // 初始化电池监控
  battery.begin();
  Serial.println("Battery monitor initialized");
}

/**
 * @brief 处理WiFi连接
 * @return true 如果WiFi连接成功，false 如果进入配网模式或连接失败
 */
bool handleWiFiConnection() {
  wifiManager.begin();
  bool wifiConnected = wifiManager.smartConnect();
  
  if (wifiConnected && !wifiManager.isConfigMode()) {
    Serial.println("WiFi connected successfully");
    timeManager.setWiFiConnected(true);
    return true;
  } else if (wifiManager.isConfigMode()) {
    handleConfigMode();
    return false;
  } else {
    Serial.println("WiFi connection failed, using cached data");
    timeManager.setWiFiConnected(false);
    return false;
  }
}

/**
 * @brief 处理配网模式
 */
void handleConfigMode() {
  Serial.println("Entered config mode");
  
  // 在屏幕显示配网信息
  String apName = wifiManager.getConfigPortalSSID();
  String ipAddress = wifiManager.getConfigPortalIP();
  epd.showConfigPortalInfo(apName, ipAddress);
  
  // 进入配网处理循环
  Serial.println("Entering config portal loop...");
  while (wifiManager.isConfigMode()) {
    wifiManager.handleConfigPortal();
    delay(100);
  }
  
  // 配网完成后会自动重启
  Serial.println("Config mode ended, restarting...");
  ESP.restart();
}

/**
 * @brief 更新天气数据
 */
void updateWeatherData() {
  if (weatherManager.shouldUpdateFromNetwork()) {
    Serial.println("Weather data is outdated, updating from network...");
    timeManager.updateNTPTime();
    weatherManager.updateWeather(true);
  } else {
    Serial.println("Weather data is recent, using cached data");
  }
}

/**
 * @brief 读取传感器数据
 * @param temperature 温度输出参数
 * @param humidity 湿度输出参数
 */
void readSensorData(float& temperature, float& humidity) {
  if (sht40.readTemperatureHumidity(temperature, humidity)) {
    Serial.println("Current Temperature: " + String(temperature) + " °C");
    Serial.println("Current Humidity: " + String(humidity) + " %RH");
  } else {
    Serial.println("Failed to read SHT40 sensor");
    temperature = NAN;
    humidity = NAN;
  }
}

/**
 * @brief 读取电池状态
 * @param batteryPercentage 电池电量百分比输出参数
 */
void readBatteryStatus(float& batteryPercentage) {
  int rawADC = battery.getRawADC();
  float batteryVoltage = battery.getBatteryVoltage();
  batteryPercentage = battery.getBatteryPercentage();
  
  Serial.println("=== 电池状态 ===");
  Serial.println("原始 ADC 值: " + String(rawADC));
  Serial.println("电池电压: " + String(batteryVoltage, 2) + " V");
  Serial.println("电池电量: " + String(batteryPercentage, 1) + " %");
  Serial.println("================");
}

/**
 * @brief 更新显示内容
 */
void updateDisplay() {
  WeatherInfo currentWeather = weatherManager.getCurrentWeather();
  DateTime currentTime = timeManager.getCurrentTime();
  
  float temperature, humidity;
  readSensorData(temperature, humidity);
  
  float batteryPercentage;
  readBatteryStatus(batteryPercentage);
  
  epd.showTimeDisplay(currentTime, currentWeather, temperature, humidity, batteryPercentage);
}

/**
 * @brief 主设置函数
 */
void setup() {
  // 初始化硬件和传感器
  initializeHardware();
  initializeRTC();
  initializeDisplay();
  initializeSensors();
  
  // 处理WiFi连接
  bool wifiConnected = handleWiFiConnection();
  
  // 如果WiFi连接成功，更新天气数据
  if (wifiConnected) {
    updateWeatherData();
  }
  
  // 更新显示
  updateDisplay();
  
  // 进入深度睡眠
  goToDeepSleep();
}

void loop() {

}

// 设置并进入深度睡眠
void goToDeepSleep() {
  Serial.println("Setting up and entering deep sleep...");
  
  // 清除之前的定时器标志和闹钟标志，防止 INT 引脚持续拉低
  rtc.clearTimerFlag();
  rtc.clearAlarmFlag();
  Serial.println("RTC interrupt flags cleared");
  
  // 设置RTC定时器唤醒时间（使用config.h中的配置）
  rtc.setTimer(RTC_TIMER_SECONDS, BM8563_TIMER_1HZ);
  
  // 启用定时器中断
  rtc.enableTimerInterrupt(true);
  Serial.println("Timer interrupt enabled");
  
  Serial.println("Entering deep sleep...");
  Serial.flush();
  
  // 等待串口输出完成
  delay(100);
  
  // 进入深度睡眠，参数 0 表示无限期睡眠直到外部唤醒
  // 实际唤醒由 RTC 定时器触发硬件复位实现
  ESP.deepSleep(0);
}

