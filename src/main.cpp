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
#include "../lib/ConfigManager/ConfigManager.h"
#include "../lib/SerialConfig/SerialConfig.h"
#include "../lib/WebConfig/WebConfig.h"
#include "../lib/Fonts/Weather_Symbols_Regular9pt7b.h"
#include "../lib/Fonts/DSEG7Modern_Bold28pt7b.h"

// 深度睡眠相关定义
#define DEEP_SLEEP_DURATION 60  // 1分钟深度睡眠（单位：秒）

// RXD 引脚定义（用于检测 Web 配置模式）
#define RXD_PIN 3  // GPIO-3 (RXD)

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

// 创建 ConfigManager 对象实例
ConfigManager configManager;

// 创建 ConfigSerial 对象实例
ConfigSerial serialConfig(&configManager);

// 创建 WebConfig 对象实例
WebConfig webConfig(&configManager);

// 创建WiFiManager对象实例
WiFiManager wifiManager;

// 实际使用的配置（将从 EEPROM 或 config.h 默认值加载）
String AMAP_API_KEY;
String cityCode;
// 创建WeatherManager对象实例（稍后初始化）
WeatherManager* weatherManager = nullptr;

// 创建SHT40对象实例
SHT40 sht40(SDA_PIN, SCL_PIN);

// 创建BatteryMonitor对象实例
BatteryMonitor battery;

// 深度睡眠相关函数声明
void goToDeepSleep();

void setup() {
  Serial.begin(74880);
  delay(100);
  
  Serial.println("\n\n=================================");
  Serial.println("    WeWeather 系统启动中...");
  Serial.println("=================================\n");
  
  // 立即初始化 RTC 并禁用定时器，防止从深度睡眠唤醒后被定时器重启
  Serial.println("初始化 RTC...");
  if (rtc.begin()) {
    Serial.println("RTC 初始化成功");
    // 立即禁用所有中断和定时器
    rtc.enableTimerInterrupt(false);
    rtc.enableAlarmInterrupt(false);
    rtc.clearTimerFlag();
    rtc.clearAlarmFlag();
    Serial.println("RTC 定时器和中断已禁用");
  } else {
    Serial.println("警告: RTC 初始化失败");
  }
  
  // 检查 RXD 引脚状态，判断是否进入 Web 配置模式
  pinMode(RXD_PIN, INPUT_PULLUP);
  delay(100); // 等待引脚稳定
  
  bool enterWebConfig = (digitalRead(RXD_PIN) == LOW);
  
  if (enterWebConfig) {
    Serial.println("\n=================================");
    Serial.println("  检测到 RXD 被拉低");
    Serial.println("  进入 Web 配置模式");
    Serial.println("=================================\n");
    
    // 初始化 ConfigManager
    configManager.begin();
    
    // 初始化墨水屏
    epd.begin();
    epd.setRotation(1);
    
    // 启动 Web 配置模式（AP SSID: weweather）
    webConfig.begin("weweather", "");
    
    // 在屏幕上显示 SSID 和 IP
    String apSSID = webConfig.getAPSSID();
    String apIP = webConfig.getAPIP();
    epd.showWebConfigInfo(apSSID, apIP);
    
    // 进入配置模式（5分钟超时）
    bool configured = webConfig.enterConfigMode(300000);
    
    if (configured) {
      Serial.println("\n配置完成，准备重启...");
      delay(2000);
      ESP.restart();
    } else {
      Serial.println("\n配置超时，退出配置模式");
      webConfig.stop();
      // 继续正常启动流程
    }
  }
  
  // 初始化 ConfigManager
  configManager.begin();
  
  // 初始化 ConfigSerial
  serialConfig.begin(74880);
  
  // 检查是否需要进入配置模式
  if (serialConfig.shouldEnterConfigMode()) {
    Serial.println("\n进入配置模式（RTC 定时器已禁用，可以安全配置）...");
    
    serialConfig.enterConfigMode(60000); // 60秒超时
    Serial.println("配置模式结束，等待 EEPROM 写入完成...");
    
    // 确保 EEPROM 写入完全完成
    delay(500);
    
    Serial.println("继续启动...\n");
  }
  
  // 加载配置
  DeviceConfig config;
  bool hasConfig = configManager.loadConfig(config);
  
  // 设置 WiFi 配置
  if (hasConfig && strlen(config.ssid) > 0) {
    Serial.println("使用 EEPROM 中的 WiFi 配置");
    WiFiConfig wifiConfig;
    strncpy(wifiConfig.ssid, config.ssid, sizeof(wifiConfig.ssid));
    strncpy(wifiConfig.password, config.password, sizeof(wifiConfig.password));
    
    if (strlen(config.macAddress) > 0) {
      strncpy(wifiConfig.macAddress, config.macAddress, sizeof(wifiConfig.macAddress));
      wifiConfig.useMacAddress = true;
    } else {
      wifiConfig.useMacAddress = false;
    }
    
    wifiConfig.timeout = 10000;
    wifiConfig.autoReconnect = true;
    wifiConfig.maxRetries = 3;
    
    wifiManager.begin(wifiConfig);
  } else {
    Serial.println("使用默认 WiFi 配置");
    wifiManager.begin();
  }
  
  // 设置高德地图 API 配置
  if (hasConfig && strlen(config.amapApiKey) > 0) {
    Serial.println("使用 EEPROM 中的高德地图配置");
    AMAP_API_KEY = String(config.amapApiKey);
    cityCode = String(config.cityCode);
  } else {
    Serial.println("使用默认高德地图配置（从 config.h 读取）");
    AMAP_API_KEY = DEFAULT_AMAP_API_KEY;
    cityCode = DEFAULT_CITY_CODE;
  }
  
  // 初始化 WeatherManager
  weatherManager = new WeatherManager(AMAP_API_KEY.c_str(), cityCode, &rtc, 512);
  weatherManager->begin();
  
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
  
  // RTC 已在 setup() 开始时初始化，这里只需初始化 TimeManager
  // 初始化TimeManager并从RTC读取时间
  timeManager.begin();
  // 判断是否需要从网络更新天气
  if (weatherManager->shouldUpdateFromNetwork()) {
    Serial.println("天气数据已过期，从网络更新...");
    
    // 如果WiFi连接成功，更新NTP时间和天气信息
    if (wifiManager.autoConnect()) {
      timeManager.setWiFiConnected(true);
      timeManager.updateNTPTime();
      weatherManager->updateWeather(true);
    } else {
      Serial.println("WiFi 连接失败，使用缓存数据");
      timeManager.setWiFiConnected(false);
    }
  } else {
    Serial.println("天气数据较新，使用缓存数据");
  }
  
  // 获取当前天气信息并显示
  WeatherInfo currentWeather = weatherManager->getCurrentWeather();
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

