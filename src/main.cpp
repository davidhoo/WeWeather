#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <EEPROM.h>
#include "../lib/BM8563/BM8563.h"
#include "../lib/GDEY029T94/GDEY029T94.h"
#include "../lib/WeatherStorage/WeatherStorage.h"
#include "Weather_Symbols_Regular9pt7b.h"
#include "DSEG7Modern_Bold28pt7b.h"

// I2C引脚定义 (根据用户提供的连接)
#define SDA_PIN 2  // GPIO-2 (D4)
#define SCL_PIN 12 // GPIO-12 (D6)

// GDEY029T94 墨水屏引脚定义
#define EPD_CS    D8
#define EPD_DC    D2
#define EPD_RST   D0
#define EPD_BUSY  D1

DateTime currentTime = {25, 9, 10, 20, 1, 34};
WeatherInfo currentWeather; // 将从EEPROM中读取
unsigned long lastMillis = 0;
unsigned long lastFullRefresh = 0;    // 上次全屏刷新时间
int lastDisplayedMinute = -1;         // 上次显示的分钟数，用于检测分钟变化
const unsigned long weatherUpdateInterval = 30 * 60 * 1000; // 30分钟更新一次天气

// 创建天气存储对象
WeatherStorage weatherStorage(512); // 512字节的EEPROM空间

// 创建BM8563对象实例
BM8563 rtc(SDA_PIN, SCL_PIN);

// 创建GDEY029T94对象实例
GDEY029T94 epd(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY);

// WiFi配置
const char* targetSSID = "Sina Plaza Office";
const char* wifiUsername = "hubo3";
const char* wifiPassword = "urtheone";
bool wifiConnected = false;
unsigned long lastWiFiCheck = 0;
const unsigned long wifiCheckInterval = 30000; // 30秒检查一次WiFi连接

void updateTime();
void connectToWiFi();
void checkWiFiConnection();
void updateNTPTime();
void updateWeatherInfo();
bool fetchWeatherData();
char mapWeatherToSymbol(const String& weather);

// BM8563 RTC 函数声明
bool readTimeFromBM8563();
bool writeTimeToBM8563(const DateTime& dt);

void setup() {
  lastMillis = millis();
  lastFullRefresh = millis();
  lastDisplayedMinute = currentTime.minute;
  
  Serial.begin(115200);
  
  // 初始化天气存储
  weatherStorage.begin();
  
  // 从EEPROM读取天气信息
  if (!weatherStorage.readWeatherInfo(currentWeather)) {
    Serial.println("Using default weather values");
    // 设置默认天气值
    currentWeather.Temperature = 23.5;
    currentWeather.Humidity = 65;
    currentWeather.Symbol = 'n';
    currentWeather.WindDirection = "北";
    currentWeather.WindSpeed = "≤3";
    currentWeather.Weather = "晴";
  }
  
  // 初始化GDEY029T94墨水屏
  epd.begin();
  epd.setRotation(1); // 调整旋转以适应128x296分辨率
  epd.setTimeFont(&DSEG7Modern_Bold28pt7b);
  epd.setWeatherSymbolFont(&Weather_Symbols_Regular9pt7b);
  
  // 初始化BM8563 RTC
  if (rtc.begin()) {
    Serial.println("BM8563 RTC initialized successfully");
  } else {
    Serial.println("Failed to initialize BM8563 RTC");
  }
  
  // 初始化WiFi连接
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  connectToWiFi();
  
  // 显示初始时间
  epd.showTimeDisplay(currentTime, currentWeather);
}
void loop() {
  unsigned long currentMillis = millis();
  
  // 检查WiFi连接状态
  checkWiFiConnection();
  
  // 每分钟刷新一次
  if (currentTime.second == 0 && currentTime.minute != lastDisplayedMinute) {
    ESP.wdtFeed();
    // 刷新屏幕前从BM8563读取时间
    readTimeFromBM8563();
    epd.showTimeDisplay(currentTime, currentWeather);
    lastDisplayedMinute = currentTime.minute;
    lastFullRefresh = currentMillis;
  }
  
  // 检查是否需要更新天气信息
  if (weatherStorage.shouldUpdateWeather(weatherUpdateInterval)) {
    // 每30分钟更新天气时，同时同步NTP时间到BM8563
    updateNTPTime();
    updateWeatherInfo();
  }
  
  updateTime();
  ESP.wdtFeed();
  delay(1000);
}


void updateTime() {
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastMillis >= 1000) {
    lastMillis = currentMillis;
    
    currentTime.second++;
    
    if (currentTime.second >= 60) {
      currentTime.second = 0;
      currentTime.minute++;
      
      if (currentTime.minute >= 60) {
        currentTime.minute = 0;
        currentTime.hour++;
        
        if (currentTime.hour >= 24) {
          currentTime.hour = 0;
          currentTime.day++;
          
          int daysInMonth = 30;
          if (currentTime.month == 2) daysInMonth = 28;
          else if (currentTime.month == 4 || currentTime.month == 6 ||
                   currentTime.month == 9 || currentTime.month == 11) daysInMonth = 30;
          else daysInMonth = 31;
          
          if (currentTime.day > daysInMonth) {
            currentTime.day = 1;
            currentTime.month++;
            
            if (currentTime.month > 12) {
              currentTime.month = 1;
              currentTime.year++;
              if (currentTime.year > 99) currentTime.year = 0;
            }
          }
        }
      }
    }
  }
}

// 连接到WiFi网络
void connectToWiFi() {
  Serial.println("Scanning for WiFi networks...");
  
  // 扫描WiFi网络
  int n = WiFi.scanNetworks();
  Serial.println("Scan done");
  
  if (n == 0) {
    Serial.println("No WiFi networks found");
    return;
  }
  
  Serial.print(n);
  Serial.println(" networks found");
  
  // 查找目标网络
  for (int i = 0; i < n; ++i) {
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(WiFi.SSID(i));
    Serial.print(" (");
    Serial.print(WiFi.RSSI(i));
    Serial.print(")");
    Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*");
    
    // 检查是否为目标SSID
    if (WiFi.SSID(i) == targetSSID) {
      Serial.println("Found target network: " + String(targetSSID));
      
      // 连接到目标网络
      WiFi.begin(targetSSID, wifiPassword);
      
      Serial.println("Connecting to WiFi...");
      unsigned long startAttemptTime = millis();
      
      // 等待连接结果，最多等待10秒
      while (WiFi.status() != WL_CONNECTED &&
             millis() - startAttemptTime < 10000) {
        delay(100);
        Serial.print(".");
      }
      
      // 检查连接结果
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        wifiConnected = true;
        
        // WiFi连接成功后更新NTP时间
        updateNTPTime();
        
        // WiFi连接成功后更新天气信息
        updateWeatherInfo();
      } else {
        Serial.println("");
        Serial.println("Failed to connect to WiFi");
        wifiConnected = false;
      }
      return;
    }
  }
  
  Serial.println("Target network not found: " + String(targetSSID));
}

// 检查WiFi连接状态
void checkWiFiConnection() {
  unsigned long currentMillis = millis();
  
  // 每隔一定时间检查WiFi连接状态
  if (currentMillis - lastWiFiCheck >= wifiCheckInterval) {
    lastWiFiCheck = currentMillis;
    
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi disconnected, trying to reconnect...");
      wifiConnected = false;
      connectToWiFi();
    } else if (!wifiConnected) {
      // WiFi已连接但标志未设置
      Serial.println("WiFi reconnected");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
      wifiConnected = true;
    }
  }
}

// 更新NTP时间
void updateNTPTime() {
  if (!wifiConnected) {
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

// 获取天气数据
bool fetchWeatherData() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, skipping weather update");
    return false;
  }

  HTTPClient http;
  WiFiClientSecure client;
  // API URL
  String url = "https://restapi.amap.com/v3/weather/weatherInfo?key=b4bed4011e9375d01423a45fba58e836&city=110108&extensions=base&output=JSON";
  
  Serial.println("Fetching weather data from: " + url);
  
  client.setInsecure(); // 跳过SSL证书验证
  http.begin(client, url);
  http.setTimeout(5000); // 5秒超时
  
  int httpResponseCode = http.GET();
  
  if (httpResponseCode == 200) {
    String payload = http.getString();
    Serial.println("Weather data received: " + payload);
    
    // 解析JSON数据
    JsonDocument doc; // 根据实际需要调整大小
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
      Serial.println("Failed to parse JSON: " + String(error.c_str()));
      http.end();
      return false;
    }
    
    // 检查status是否为"1"
    String status = doc["status"].as<String>();
    if (status != "1") {
      Serial.println("API returned error status: " + status);
      http.end();
      return false;
    }
    
    // 获取lives数据
    JsonObject lives = doc["lives"][0];
    
    // 更新WeatherInfo
    currentWeather.Temperature = lives["temperature"].as<float>();
    currentWeather.Humidity = lives["humidity"].as<int>();
    currentWeather.WindDirection = lives["winddirection"].as<String>();
    currentWeather.WindSpeed = lives["windpower"].as<String>();
    currentWeather.Weather = lives["weather"].as<String>();
    
    // 根据天气状况设置符号
    currentWeather.Symbol = mapWeatherToSymbol(currentWeather.Weather);
    
    Serial.println("Weather updated successfully");
    Serial.println("Temperature: " + String(currentWeather.Temperature));
    Serial.println("Humidity: " + String(currentWeather.Humidity));
    Serial.println("Wind Direction: " + currentWeather.WindDirection);
    Serial.println("Wind Speed: " + currentWeather.WindSpeed);
    Serial.println("Weather: " + currentWeather.Weather);
    Serial.println("Symbol: " + String(currentWeather.Symbol));
    
    http.end();
    return true;
  } else {
    Serial.println("HTTP request failed with code: " + String(httpResponseCode));
    http.end();
    return false;
  }
}

// 更新天气信息
void updateWeatherInfo() {
  Serial.println("Updating weather information...");
  
  if (fetchWeatherData()) {
    // 天气更新成功，保存到EEPROM
    if (weatherStorage.writeWeatherInfo(currentWeather)) {
      Serial.println("Weather data saved to EEPROM successfully");
    } else {
      Serial.println("Failed to save weather data to EEPROM");
    }
    // 刷新显示
    epd.showTimeDisplay(currentTime, currentWeather);
  }
}
// 将天气状况映射到符号
char mapWeatherToSymbol(const String& weather) {
  // Weather symbol mapping:
  // n=晴(sunny), d=雪(snow), m=雨(rain), l=雾(fog), c=阴(overcast), o=多云(cloudy), k=雷雨(thunderstorm)
  
  if (weather.indexOf("晴") >= 0) {
    return 'n'; // 晴天
  } else if (weather.indexOf("雷") >= 0 && weather.indexOf("雨") >= 0) {
    return 'k'; // 雷雨
  } else if (weather.indexOf("雪") >= 0) {
    return 'd'; // 雪
  } else if (weather.indexOf("雨") >= 0) {
    return 'm'; // 雨
  } else if (weather.indexOf("雷") >= 0) {
    return 'a'; // 雷
  } else if (weather.indexOf("雾") >= 0) {
    return 'l'; // 雾
  } else if (weather.indexOf("阴") >= 0) {
    return 'c'; // 阴
  } else if (weather.indexOf("多云") >= 0) {
    return 'o'; // 多云
  }else if (weather.indexOf("少云") >= 0) {
    return 'p'; // 少云
  } else if (weather.indexOf("晴间多云") >= 0) {
    return 'c'; // 晴间多云
  } else if (weather.indexOf("风") >= 0) {
    return 'f'; // 风
  } else if (weather.indexOf("冷") >= 0) {
    return 'e'; // 冷
  } else if (weather.indexOf("热") >= 0) {
    return 'h'; // 热
  } else {
    return 'n'; // 默认晴天 abcdefghijklmnop
  }
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
