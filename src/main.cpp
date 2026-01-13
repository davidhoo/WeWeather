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
#include "../lib/ConfigManager/ConfigManager.h"
#include "../lib/SerialConfigManager/SerialConfigManager.h"
#include "../lib/WebConfigManager/WebConfigManager.h"
#include "../lib/UnifiedConfigManager/UnifiedConfigManager.h"
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

// 创建统一配置管理器实例
UnifiedConfigManager unifiedConfigManager(512);

// WeatherManager 指针，将在初始化时创建
WeatherManager* weatherManager = nullptr;

// 创建SHT40对象实例
SHT40 sht40(I2C_SDA_PIN, I2C_SCL_PIN);

// 创建BatteryMonitor对象实例
BatteryMonitor battery;
// 创建ConfigManager对象实例
ConfigManager<ConfigData> configManager;

// 创建SerialConfigManager对象实例
SerialConfigManager serialConfigManager(&configManager);

// 创建WebConfigManager对象实例
WebConfigManager webConfigManager(&configManager);

// 函数声明
void initializeManagers();
void initializeSensors();
void initializeDisplay();
void initializeRTC();
void initializeTimeManager();
bool connectAndUpdateWiFi();
void updateAndDisplay();
void goToDeepSleep();

// 配置模式相关函数声明
bool checkConfigMode();
void enterConfigMode();
void clearRTCWakeupSettings();
void showConfigDisplay();
void startAPWebConfigService();
void exitConfigMode();

/**
 * @brief 初始化各种管理器
 * 注意：TimeManager 需要在 RTC 初始化之后才能调用
 */
void initializeManagers() {
  // 初始化统一配置管理器
  unifiedConfigManager.begin();
  
  // 检查是否需要清除EEPROM（API Key字段损坏）
  String apiKey = unifiedConfigManager.getAmapApiKey();
  if (apiKey.length() > 32 || apiKey.indexOf(0xFF) != -1) {
    LOG_INFO("EEPROM API Key data corrupted, clearing EEPROM...");
    unifiedConfigManager.clearEEPROMConfig();
    LOG_INFO("EEPROM cleared, using default configuration");
  }
  
  // 从配置管理器获取API配置
  apiKey = unifiedConfigManager.getAmapApiKey();
  String cityCode = unifiedConfigManager.getCityCode();
  
  // 详细调试信息
  LOG_INFO_UTF8("=== 配置调试信息 ===");
  LOG_INFO_UTF8("Raw API Key from UnifiedConfigManager: '%s'", apiKey.c_str());
  LOG_INFO_UTF8("Raw City Code from UnifiedConfigManager: '%s'", cityCode.c_str());
  LOG_INFO_UTF8("API Key length: %d", apiKey.length());
  LOG_INFO_UTF8("City Code length: %d", cityCode.length());
  
  // 检查每个字符的十六进制值
  LOG_INFO_UTF8("API Key hex dump:");
  for (int i = 0; i < apiKey.length() && i < 20; i++) {
    Serial.print(apiKey.charAt(i), HEX);
    Serial.print(" ");
  }
  Serial.println();
  
  LOG_INFO_UTF8("City Code hex dump:");
  for (int i = 0; i < cityCode.length() && i < 20; i++) {
    Serial.print(cityCode.charAt(i), HEX);
    Serial.print(" ");
  }
  Serial.println();
  
  LOG_INFO_F("Using API Key: %s", apiKey.length() > 0 ? "***" : "未设置");
  LOG_INFO_UTF8("Using City Code: %s", cityCode.c_str());
  LOG_INFO_UTF8("=== 调试信息结束 ===");
  
  // 动态创建WeatherManager实例
  weatherManager = new WeatherManager(apiKey.c_str(), cityCode, &rtc, 512);
  
  // 初始化WeatherManager
  weatherManager->begin();
}

/**
 * @brief 初始化TimeManager
 * 必须在RTC初始化之后调用，因为需要从RTC读取时间
 */
void initializeTimeManager() {
  // 初始化TimeManager并从RTC读取时间
  timeManager.begin();
}

/**
 * @brief 初始化传感器
 */
void initializeSensors() {
  // 初始化SHT40温湿度传感器
  if (sht40.begin()) {
    LOG_INFO("SHT40 initialized successfully");
  } else {
    LOG_ERROR("Failed to initialize SHT40");
  }
}

/**
 * @brief 初始化显示屏
 * 旋转角度以适应 128x296 分辨率的横向显示
 */
void initializeDisplay() {
  epd.begin();
  epd.setRotation(DISPLAY_ROTATION);
  epd.setTimeFont(&DSEG7Modern_Bold28pt7b);
  epd.setWeatherSymbolFont(&Weather_Symbols_Regular9pt7b);
}

/**
 * @brief 初始化RTC时钟
 * 清除中断标志，防止 INT 引脚持续拉低导致无法进入深度睡眠
 */
void initializeRTC() {
  if (rtc.begin()) {
    LOG_INFO("BM8563 RTC initialized successfully");
    
    // 重置所有中断标志和禁用中断，防止 INT 引脚持续拉低
    rtc.resetInterrupts();
    LOG_INFO("RTC interrupts reset and disabled");
  } else {
    LOG_ERROR("Failed to initialize BM8563 RTC");
  }
}

/**
 * @brief 连接WiFi并更新数据
 * @return true 如果连接成功，false 如果连接失败
 */
bool connectAndUpdateWiFi() {
  // 判断是否需要从网络更新天气
  if (weatherManager->shouldUpdateFromNetwork()) {
    LOG_INFO("Weather data is outdated, updating from network...");
    
    // 从统一配置管理器获取WiFi配置
    String ssid = unifiedConfigManager.getWiFiSSID();
    String password = unifiedConfigManager.getWiFipassword();
    String macAddress = unifiedConfigManager.getMacAddress();
    
    // 设置WiFi配置
    WiFiConfig wifiConfig = {};
    strncpy(wifiConfig.ssid, ssid.c_str(), sizeof(wifiConfig.ssid) - 1);
    strncpy(wifiConfig.password, password.c_str(), sizeof(wifiConfig.password) - 1);
    strncpy(wifiConfig.macAddress, macAddress.c_str(), sizeof(wifiConfig.macAddress) - 1);
    wifiConfig.timeout = WIFI_CONNECT_TIMEOUT;
    wifiConfig.autoReconnect = true;
    wifiConfig.maxRetries = 3;
    wifiConfig.useMacAddress = ENABLE_CUSTOM_MAC;
    
    // 初始化WiFi连接（使用统一配置管理器的配置）
    wifiManager.begin(wifiConfig);
    
    // 如果WiFi连接成功，更新NTP时间和天气信息
    if (wifiManager.autoConnect()) {
      timeManager.setWiFiConnected(true);
      timeManager.updateNTPTime();
      weatherManager->updateWeather(true);
      return true;
    } else {
      LOG_WARN("WiFi connection failed, using cached data");
      timeManager.setWiFiConnected(false);
      return false;
    }
  } else {
    LOG_INFO("Weather data is recent, using cached data");
    return true; // 使用缓存数据也算成功
  }
}

/**
 * @brief 更新传感器数据并显示到屏幕
 */
void updateAndDisplay() {
  // 获取当前天气信息和时间
  WeatherInfo currentWeather = weatherManager->getCurrentWeather();
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
  
  // 显示到屏幕
  epd.showTimeDisplay(currentTime, currentWeather, temperature, humidity, batteryPercentage);
}
void setup() {
  // 初始化串口通信
  serialConfigManager.initializeSerial();
  LOG_INFO("System starting up...");
  
  // 检查是否需要进入配置模式
  if (checkConfigMode()) {
    // 进入配置模式
    initializeDisplay();  // 配置模式需要显示屏
    initializeRTC();      // 配置模式需要RTC来清除唤醒设置
    enterConfigMode();    // 进入配置模式（不会返回）
    return;
  }
  
  // 正常运行模式
  initializeManagers();
  initializeSensors();
  initializeDisplay();
  initializeRTC();
  initializeTimeManager();  // 必须在RTC初始化之后
  
  connectAndUpdateWiFi();
  updateAndDisplay();
  
  goToDeepSleep();
}

void loop() {

}



// 设置并进入深度睡眠
void goToDeepSleep() {
  LOG_INFO("Setting up and entering deep sleep...");
  
  // 配置 RTC 定时器在指定时间后通过 INT 引脚唤醒 ESP8266
  rtc.setupWakeupTimer(RTC_TIMER_SECONDS);
  LOG_INFO("RTC wakeup timer configured");
  
  LOG_INFO("Entering deep sleep...");
  Serial.flush();
  
  // 等待串口输出完成
  delay(100);
  
  // 进入深度睡眠，参数 0 表示无限期睡眠直到外部唤醒
  // 实际唤醒由 RTC 定时器触发硬件复位实现
  ESP.deepSleep(0);
}

/**
 * @brief 检查是否需要进入配置模式
 * 检测RXD引脚是否被拉低
 * @return true 如果需要进入配置模式，false 否则
 */
bool checkConfigMode() {
  // 设置RXD引脚为输入模式，启用内部上拉电阻
  pinMode(RXD_PIN, INPUT_PULLUP);
  delay(10); // 等待引脚状态稳定
  
  // 读取RXD引脚状态，如果被拉低则进入配置模式
  bool isConfigMode = (digitalRead(RXD_PIN) == LOW);
  
  if (isConfigMode) {
    LOG_INFO("RXD pin is LOW, entering configuration mode");
  } else {
    LOG_INFO("RXD pin is HIGH, normal operation mode");
  }
  
  return isConfigMode;
}

/**
 * @brief 进入配置模式
 * 清除RTC唤醒设置，显示配置信息，启动配置服务
 */
void enterConfigMode() {
  LOG_INFO("Entering configuration mode...");
  
  // 1. 重新配置串口
  serialConfigManager.reconfigureSerial();
  
  // 2. 清除RTC的定时唤醒设置
  clearRTCWakeupSettings();
  
  // 3. 初始化ConfigManager
  configManager.begin();
  
  // 4. 启动配置服务
  startAPWebConfigService();
  serialConfigManager.startConfigService();
  
  // 5. 在屏幕显示配置信息提示（需要先启动服务获取IP）
  showConfigDisplay();
  
  LOG_INFO("Configuration mode services started");
  // 配置模式下保持运行，不进入深度睡眠
  while (true) {
    // 处理串口命令
    serialConfigManager.processInput();
    
    // 处理Web请求
    webConfigManager.handleClient();
    
    delay(100); // 减少延时，提高响应性
  }
}

/**
 * @brief 清除RTC的定时唤醒设置
 * 防止在配置模式期间被RTC自动唤醒重启系统
 */
void clearRTCWakeupSettings() {
  LOG_INFO("Clearing RTC wakeup settings...");
  
  // 清除定时器设置
  rtc.clearTimer();
  
  // 禁用定时器中断
  rtc.enableTimerInterrupt(false);
  
  // 清除所有中断标志
  rtc.resetInterrupts();
  
  LOG_INFO("RTC wakeup settings cleared");
}

/**
 * @brief 在屏幕显示配置信息提示
 * 显示AP名称和IP地址
 */
void showConfigDisplay() {
  LOG_INFO("Showing configuration display...");
  
  // 获取AP信息
  const char* apName = "WeWeather";
  IPAddress apIP = WiFi.softAPIP();
  String apIPStr = apIP.toString();
  
  // 调用显示屏的配置显示方法
  epd.showConfigDisplay(apName, apIPStr.c_str());
  
  LOG_INFO("Configuration display shown");
}

/**
 * @brief 启动AP+WEB配置服务
 * 启动名为"WeWeather"的AP热点
 */
void startAPWebConfigService() {
  LOG_INFO("Starting AP+Web configuration service...");
  
  // 启动WiFi AP模式
  const char* apName = "WeWeather";
  const char* apPassword = ""; // 无密码的开放热点
  
  WiFi.mode(WIFI_AP);
  bool apStarted = WiFi.softAP(apName, apPassword);
  
  if (apStarted) {
    IPAddress apIP = WiFi.softAPIP();
    LOG_INFO("AP started successfully");
    LOG_INFO_F("AP Name: %s", apName);
    LOG_INFO_F("AP IP: %s", apIP.toString().c_str());
    
    // 启动Web配置服务
    if (webConfigManager.startConfigService()) {
      LOG_INFO("Web configuration service started successfully");
    } else {
      LOG_ERROR("Failed to start web configuration service");
    }
  } else {
    LOG_ERROR("Failed to start AP");
  }
  
  LOG_INFO("AP+Web configuration service started");
}


