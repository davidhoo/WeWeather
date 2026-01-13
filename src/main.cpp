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

// 创建ConfigManager对象实例
ConfigManager<ConfigData> configManager;

// 函数声明
void initializeSerial();
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
void startSerialConfigService();

// 串口配置服务相关函数声明
void processSerialCommand();
void showConfig();
void setConfig(String key, String value);
void clearConfig();
void showHelp();
void exitConfigMode();

/**
 * @brief 初始化串口通信
 * ESP8266 ROM bootloader 使用 74880 波特率，保持一致便于查看启动信息
 */
void initializeSerial() {
  LogManager::begin(SERIAL_BAUD_RATE, LOG_INFO);
  LOG_INFO("System starting up...");
}

/**
 * @brief 初始化各种管理器
 * 注意：TimeManager 需要在 RTC 初始化之后才能调用
 */
void initializeManagers() {
  // 初始化WeatherManager
  weatherManager.begin();
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
  if (weatherManager.shouldUpdateFromNetwork()) {
    LOG_INFO("Weather data is outdated, updating from network...");
    
    // 初始化WiFi连接（使用默认配置）
    wifiManager.begin();
    
    // 如果WiFi连接成功，更新NTP时间和天气信息
    if (wifiManager.autoConnect()) {
      timeManager.setWiFiConnected(true);
      timeManager.updateNTPTime();
      weatherManager.updateWeather(true);
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
  
  // 显示到屏幕
  epd.showTimeDisplay(currentTime, currentWeather, temperature, humidity, batteryPercentage);
}
void setup() {
  initializeSerial();
  
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
  
  // 1. 重新配置RXD引脚为串口功能并重新初始化串口
  pinMode(RXD_PIN, INPUT);  // 移除上拉电阻，恢复串口功能
  
  // 重新初始化串口以确保RXD引脚正常工作
  Serial.end();
  delay(100);
  Serial.begin(SERIAL_BAUD_RATE);
  delay(100);
  
  LOG_INFO("RXD pin reconfigured and serial reinitialized");
  
  // 2. 清除RTC的定时唤醒设置
  clearRTCWakeupSettings();
  
  // 3. 初始化ConfigManager
  configManager.begin();
  
  // 3. 启动配置服务
  startAPWebConfigService();
  startSerialConfigService();
  
  // 4. 在屏幕显示配置信息提示（需要先启动服务获取IP）
  showConfigDisplay();
  
  LOG_INFO("Configuration mode services started");
  LOG_INFO("Type 'help' for available commands");
  
  // 配置模式下保持运行，不进入深度睡眠
  while (true) {
    // 处理串口命令
    int available = Serial.available();
    if (available > 0) {
      LOG_INFO_F("Serial data available: %d bytes, processing command...", available);
      processSerialCommand();
    }
    
    delay(100); // 减少延时，提高响应性
    // 这里可以添加其他配置模式的逻辑
    // 例如：处理Web请求等
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
  } else {
    LOG_ERROR("Failed to start AP");
  }
  
  // TODO: 启动Web服务器和配置界面（后续实现）
  
  LOG_INFO("AP+Web configuration service started");
}

/**
 * @brief 启动串口配置服务
 * 空实现，后续完善
 */
void startSerialConfigService() {
  LOG_INFO("Starting serial configuration service...");
  
  // TODO: 实现串口配置功能
  // 例如：
  // - 监听串口命令
  // - 解析配置参数
  // - 保存配置到存储
  
  LOG_INFO("Serial configuration service started");
  
  // 显示欢迎信息和帮助提示
  Serial.println();
  Serial.println("=== WeWeather Serial Configuration ===");
  Serial.println("Type 'help' for available commands");
  Serial.print("> ");
}

/**
 * @brief 处理串口命令
 * 读取串口输入并解析命令
 */
void processSerialCommand() {
  // 等待完整的命令行输入
  String command = "";
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      break;
    }
    command += c;
    delay(1); // 短暂延时，确保接收完整
  }
  
  // 清除剩余的换行符
  while (Serial.available() > 0 && (Serial.peek() == '\n' || Serial.peek() == '\r')) {
    Serial.read();
  }
  
  command.trim(); // 去除首尾空白字符
  
  // 调试输出
  Serial.println("Received command: '" + command + "' (length: " + String(command.length()) + ")");
  
  if (command.length() == 0) {
    Serial.print("> ");
    return;
  }
  
  // 解析命令和参数
  int spaceIndex = command.indexOf(' ');
  String cmd = (spaceIndex > 0) ? command.substring(0, spaceIndex) : command;
  String args = (spaceIndex > 0) ? command.substring(spaceIndex + 1) : "";
  
  cmd.toLowerCase();
  
  // 调试输出
  Serial.println("Parsed command: '" + cmd + "', args: '" + args + "'");
  
  // 执行命令
  if (cmd == "show") {
    showConfig();
  } else if (cmd == "set") {
    // 解析 set 命令的参数：set key value
    int argSpaceIndex = args.indexOf(' ');
    if (argSpaceIndex > 0) {
      String key = args.substring(0, argSpaceIndex);
      String value = args.substring(argSpaceIndex + 1);
      setConfig(key, value);
    } else {
      Serial.println("Usage: set <key> <value>");
      Serial.println("Keys: ssid, password, apikey, citycode, mac");
    }
  } else if (cmd == "clear") {
    clearConfig();
  } else if (cmd == "help") {
    showHelp();
  } else if (cmd == "exit") {
    exitConfigMode();
  } else {
    Serial.println("Unknown command: '" + cmd + "'");
    Serial.println("Type 'help' for available commands");
  }
  
  Serial.print("> ");
}

/**
 * @brief 显示当前配置
 */
void showConfig() {
  Serial.println("=== Current Configuration ===");
  
  ConfigData config;
  if (configManager.read(config)) {
    Serial.println("SSID: " + String(config.wifiSSID));
    Serial.println("password: " + String(config.wifiPassword));
    Serial.println("API Key: " + String(config.amapApiKey));
    Serial.println("City Code: " + String(config.cityCode));
    Serial.println("MAC Address: " + String(config.macAddress));
  } else {
    Serial.println("No valid configuration found or failed to read");
  }
  
  Serial.println("=============================");
}

/**
 * @brief 设置配置项
 * @param key 配置键
 * @param value 配置值
 */
void setConfig(String key, String value) {
  key.toLowerCase();
  
  ConfigData config;
  // 尝试读取现有配置，如果失败则使用默认值
  if (!configManager.read(config)) {
    // 初始化为空配置
    memset(&config, 0, sizeof(config));
  }
  
  bool validKey = true;
  
  if (key == "ssid") {
    strncpy(config.wifiSSID, value.c_str(), sizeof(config.wifiSSID) - 1);
    config.wifiSSID[sizeof(config.wifiSSID) - 1] = '\0';
  } else if (key == "password") {
    strncpy(config.wifiPassword, value.c_str(), sizeof(config.wifiPassword) - 1);
    config.wifiPassword[sizeof(config.wifiPassword) - 1] = '\0';
  } else if (key == "apikey") {
    strncpy(config.amapApiKey, value.c_str(), sizeof(config.amapApiKey) - 1);
    config.amapApiKey[sizeof(config.amapApiKey) - 1] = '\0';
  } else if (key == "citycode") {
    strncpy(config.cityCode, value.c_str(), sizeof(config.cityCode) - 1);
    config.cityCode[sizeof(config.cityCode) - 1] = '\0';
  } else if (key == "mac") {
    strncpy(config.macAddress, value.c_str(), sizeof(config.macAddress) - 1);
    config.macAddress[sizeof(config.macAddress) - 1] = '\0';
  } else {
    validKey = false;
    Serial.println("Invalid key: " + key);
    Serial.println("Valid keys: ssid, password, apikey, citycode, mac");
  }
  
  if (validKey) {
    Serial.println("Set " + key + " = " + value);
    
    // 直接写入EEPROM
    if (configManager.write(config)) {
      Serial.println("Configuration saved successfully");
    } else {
      Serial.println("Failed to save configuration");
    }
  }
}

/**
 * @brief 清除配置
 * 只清除系统配置字段（SSID、password、citycode、apikey、mac），保持天气数据不变
 */
void clearConfig() {
  ConfigData config;
  
  // 先读取现有配置，保持天气数据
  if (configManager.read(config)) {
    // 只清除系统配置字段，保持天气数据不变
    memset(config.wifiSSID, 0, sizeof(config.wifiSSID));
    memset(config.wifiPassword, 0, sizeof(config.wifiPassword));
    memset(config.amapApiKey, 0, sizeof(config.amapApiKey));
    memset(config.cityCode, 0, sizeof(config.cityCode));
    memset(config.macAddress, 0, sizeof(config.macAddress));
    
    // 写回配置，天气数据保持不变
    if (configManager.write(config)) {
      Serial.println("System configuration cleared (weather data preserved)");
    } else {
      Serial.println("Failed to clear configuration");
    }
  } else {
    Serial.println("No configuration found to clear");
  }
}

/**
 * @brief 显示帮助信息
 */
void showHelp() {
  Serial.println("=== Available Commands ===");
  Serial.println("show                    - Display current configuration");
  Serial.println("set <key> <value>       - Set and save configuration value");
  Serial.println("  Keys: ssid, password, apikey, citycode, mac");
  Serial.println("clear                   - Clear all configuration");
  Serial.println("help                    - Show this help message");
  Serial.println("exit                    - Exit configuration mode (restart system)");
  Serial.println("==========================");
  Serial.println();
  Serial.println("Examples:");
  Serial.println("  set ssid MyWiFi");
  Serial.println("  set password myPassword");
  Serial.println("  set apikey your_amap_api_key");
  Serial.println("  set citycode 110108");
  Serial.println("  set mac AA:BB:CC:DD:EE:FF");
}

/**
 * @brief 退出配置模式
 * 重启系统以应用新配置
 */
void exitConfigMode() {
  Serial.println("Exiting configuration mode...");
  Serial.println("System will restart in 3 seconds...");
  
  for (int i = 3; i > 0; i--) {
    Serial.println(String(i) + "...");
    delay(1000);
  }
  
  Serial.println("Restarting...");
  Serial.flush();
  ESP.restart();
}

