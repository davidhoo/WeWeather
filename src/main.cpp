#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "Weather_Symbols_Regular9pt7b.h"
#include "DSEG7Modern_Bold28pt7b.h"
#include <ArduinoJson.h>
#include <Wire.h>

// BM8563 RTC 常量定义
#define BM8563_I2C_ADDRESS 0x51
#define BM8563_REG_SECONDS 0x02
#define BM8563_REG_MINUTES 0x03
#define BM8563_REG_HOURS   0x04
#define BM8563_REG_DAYS    0x05
#define BM8563_REG_MONTHS  0x07
#define BM8563_REG_YEARS   0x08
#define BM8563_REG_WEEKDAYS 0x06

// I2C引脚定义 (根据用户提供的连接)
#define SDA_PIN 2  // GPIO-2 (D4)
#define SCL_PIN 12 // GPIO-12 (D6)

// GDEY029T94 2.9寸黑白墨水屏 128x296 (使用SSD1680控制器)
struct DateTime {
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;
};

// Weather information structure
struct WeatherInfo {
  float Temperature;    // Temperature in Celsius
  int Humidity;         // Humidity percentage
  char Symbol;          // Weather symbol character
  String WindDirection; // 风向
  String WindSpeed;     // 风速
  String Weather;       // 天气状况
  // Weather symbol mapping:
  // n=晴(sunny), d=雪(snow), m=雨(rain), l=雾(fog), c=阴(overcast), o=多云(cloudy), k=雷雨(thunderstorm)
};

DateTime currentTime = {25, 9, 10, 20, 1, 34};
WeatherInfo currentWeather = {23.5, 65, 'n', "北", "≤3", "晴"}; // 23.5°C, 65% humidity, sunny
unsigned long lastMillis = 0;
unsigned long lastFullRefresh = 0;    // 上次全屏刷新时间
int lastDisplayedMinute = -1;         // 上次显示的分钟数，用于检测分钟变化
unsigned long lastWeatherUpdate = 0;  // 上次天气更新时间
const unsigned long weatherUpdateInterval = 30 * 60 * 1000; // 30分钟更新一次天气
const GFXfont* timeFont = &DSEG7Modern_Bold28pt7b;  // 时间显示字体

// WiFi配置
const char* targetSSID = "Sina Plaza Office";
const char* wifiUsername = "hubo3";
const char* wifiPassword = "urtheone";
bool wifiConnected = false;
unsigned long lastWiFiCheck = 0;
const unsigned long wifiCheckInterval = 30000; // 30秒检查一次WiFi连接

#define EPD_CS    D8
#define EPD_DC    D2
#define EPD_RST   D0
#define EPD_BUSY  D1
// 原始定义可能存在width/height混淆，尝试使用WIDTH而不是HEIGHT
GxEPD2_BW<GxEPD2_290_GDEY029T94, GxEPD2_290_GDEY029T94::WIDTH> display(GxEPD2_290_GDEY029T94(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

void showTimeDisplay();
void updateTime();
String getFormattedTime();
String getFormattedDate();
String getWeatherInfo();
char getWeatherSymbol();
String getDayOfWeek(int year, int month, int day);
void processSerialInput();
void setTimeFromTimestamp(unsigned long timestamp);
DateTime timestampToDateTime(unsigned long timestamp);
void connectToWiFi();
void checkWiFiConnection();
void updateNTPTime();
void updateWeatherInfo();
bool fetchWeatherData();
char mapWeatherToSymbol(const String& weather);
String translateWindDirection(const String& chineseDirection);
String formatWindSpeed(const String& windSpeed);

// BM8563 RTC 函数声明
void initBM8563();
bool readTimeFromBM8563();
bool writeTimeToBM8563(const DateTime& dt);
uint8_t decToBcd(uint8_t val);
uint8_t bcdToDec(uint8_t val);

// 8像素对齐辅助函数 - SSD1680控制器要求x坐标必须8像素对齐
int alignToPixel8(int x) {
  return (x / 8) * 8;
}

void setup() {
  lastMillis = millis();
  lastFullRefresh = millis();
  lastDisplayedMinute = currentTime.minute;
  
  // 在初始化显示之前设置旋转，确保第一次显示正确旋转
  display.setRotation(1); // 调整旋转以适应128x296分辨率
  display.init();
  
  Serial.begin(115200);
  
  // 初始化I2C和BM8563 RTC
  Wire.begin(SDA_PIN, SCL_PIN);
  initBM8563();
  
  // 初始化WiFi连接
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  connectToWiFi();
  
  showTimeDisplay();
}
void loop() {
  unsigned long currentMillis = millis();
  
  // 处理串口输入
  processSerialInput();
  
  // 检查WiFi连接状态
  checkWiFiConnection();
  // 每分钟刷新一次
  if (currentTime.second == 0 && currentTime.minute != lastDisplayedMinute) {
    ESP.wdtFeed();
    // 刷新屏幕前从BM8563读取时间
    readTimeFromBM8563();
    showTimeDisplay();
    lastDisplayedMinute = currentTime.minute;
    lastFullRefresh = currentMillis;
  }
  
  // 每30分钟更新一次天气信息
  if (currentMillis - lastWeatherUpdate >= weatherUpdateInterval) {

    // 每30分钟更新天气时，同时同步NTP时间到BM8563
    updateNTPTime();
    updateWeatherInfo();
  }
  
  updateTime();
  ESP.wdtFeed();
  delay(1000);
}

void showTimeDisplay() {
  String timeStr = getFormattedTime();
  String dateStr = getFormattedDate();
  String weatherStr = getWeatherInfo();
  
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    
    // Display weather info (using small font, left aligned)
    display.setFont(&FreeMonoBold9pt7b);
    int weatherX = alignToPixel8(10); // Left aligned, 8-pixel aligned from left edge
    int weatherY = 15; // Top position
    
    display.setCursor(weatherX, weatherY);
    display.print(weatherStr);
    
    // Display weather symbol (right aligned)
    display.setFont(&Weather_Symbols_Regular9pt7b);
    char symbolStr[2] = {getWeatherSymbol(), '\0'};
    int16_t sbx, sby;
    uint16_t sbw, sbh;
    display.getTextBounds(symbolStr, 0, 0, &sbx, &sby, &sbw, &sbh);
    
    int symbolX = alignToPixel8(display.width() - sbw - 10); // Right aligned, 8-pixel aligned
    int symbolY = weatherY; // Same Y position as weather info
    
    display.setCursor(symbolX, symbolY);
    display.print(symbolStr);
    
    // Draw line below weather info
    int topLineY = weatherY + 5;
    display.drawLine(alignToPixel8(10), topLineY, display.width() - alignToPixel8(10), topLineY, GxEPD_BLACK);
    
    // Display time (using large font, centered with fixed position)
    display.setFont(timeFont);
    
    // 使用固定的时间字符串"00:00"来计算居中位置，确保位置不变
    String fixedTimeStr = "00:00";
    int16_t tbx, tby;
    uint16_t tbw, tbh;
    display.getTextBounds(fixedTimeStr, 0, 0, &tbx, &tby, &tbw, &tbh);
    
    int timeX = alignToPixel8((display.width() - tbw) / 2); // Center aligned, 8-pixel aligned
    int timeY = topLineY + tbh + 10; // Below the top line (减少间距)
    
    display.setCursor(timeX, timeY);
    display.print(timeStr);
    
    // Draw line below time
    int bottomLineY = timeY + 10;
    display.drawLine(alignToPixel8(10), bottomLineY, display.width() - alignToPixel8(10), bottomLineY, GxEPD_BLACK);
    
    // Display date (using small font, left aligned)
    display.setFont(&FreeMonoBold9pt7b);
    int dateX = alignToPixel8(10); // Left aligned, 8-pixel aligned from left edge
    int dateY = bottomLineY + 20; // 20 pixels below the line (减少间距)
    
    display.setCursor(dateX, dateY);
    display.print(dateStr);
    
  } while (display.nextPage());
  
  display.hibernate();
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

String getFormattedTime() {
  char timeString[6];
  sprintf(timeString, "%02d:%02d", currentTime.hour, currentTime.minute);
  
  return String(timeString);
}

String getFormattedDate() {
  // 获取完整年份（2000 + 两位数年份）
  int fullYear = 2000 + currentTime.year;
  
  // 获取星期几
  String dayOfWeek = getDayOfWeek(fullYear, currentTime.month, currentTime.day);
  
  char dateString[50];
  sprintf(dateString, "%04d/%02d/%02d %s", fullYear, currentTime.month, currentTime.day, dayOfWeek.c_str());
  
  return String(dateString);
}

String getDayOfWeek(int year, int month, int day) {
  // 使用Zeller公式计算星期几
  if (month < 3) {
    month += 12;
    year--;
  }
  
  int k = year % 100;
  int j = year / 100;
  
  int h = (day + ((13 * (month + 1)) / 5) + k + (k / 4) + (j / 4) - 2 * j) % 7;
  
  // 转换为星期几字符串（0=周六，1=周日，2=周一...）
  String days[] = {"Saturday", "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday"};
  return days[h];
}
String getWeatherInfo() {
  // Format weather information for display
  String weatherString = "";
  
  // 按照新格式显示天气信息: 22C 46% NortheEast ≤3
  weatherString += String(currentWeather.Temperature, 0) + "C ";
  weatherString += String(currentWeather.Humidity) + "% ";
  weatherString += translateWindDirection(currentWeather.WindDirection) + " ";
  weatherString += currentWeather.WindSpeed;
  
  return weatherString;
}

char getWeatherSymbol() {
  // Return weather symbol character
  return currentWeather.Symbol;
}
// 处理串口输入
void processSerialInput() {
  static String inputString = "";
  
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    
    if (inChar == '\n') {
      // 解析输入的时间戳
      if (inputString.startsWith("T")) {
        String timestampStr = inputString.substring(1);
        timestampStr.trim();
        
        // 按UTC处理时间戳
        unsigned long timestamp = timestampStr.toInt();
        if (timestamp > 0) {
          // UTC时间转换为本地时间（UTC+8）
          unsigned long localTimestamp = timestamp + (8 * 3600);
          setTimeFromTimestamp(localTimestamp);
        }
      }
      inputString = "";
    } else {
      inputString += inChar;
    }
  }
}

// 从Unix时间戳设置时间
void setTimeFromTimestamp(unsigned long timestamp) {
  DateTime newTime = timestampToDateTime(timestamp);
  currentTime = newTime;
  lastMillis = millis();
  
  // 立即刷新显示并重置刷新计时器
  unsigned long currentMillis = millis();
  showTimeDisplay();
  lastFullRefresh = currentMillis;
  lastDisplayedMinute = currentTime.minute;
}

// 判断是否为闰年
bool isLeapYear(int year) {
  return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

// 获取指定月份的天数
int daysInMonth(int month, int year) {
  switch (month) {
    case 2: return isLeapYear(year) ? 29 : 28;
    case 4: case 6: case 9: case 11: return 30;
    default: return 31;
  }
}

// 将Unix时间戳转换为DateTime结构
DateTime timestampToDateTime(unsigned long timestamp) {
  DateTime dt;
  
  // 计算秒、分、时
  dt.second = timestamp % 60;
  timestamp /= 60;
  dt.minute = timestamp % 60;
  timestamp /= 60;
  dt.hour = timestamp % 24;
  timestamp /= 24;
  
  // 计算年、月、日（从1970年1月1日开始计算）
  int days = timestamp;
  dt.year = 1970;
  dt.month = 1;
  dt.day = 1;
  
  // 计算年份
  while (days >= (isLeapYear(dt.year) ? 366 : 365)) {
    int daysInYear = isLeapYear(dt.year) ? 366 : 365;
    days -= daysInYear;
    dt.year++;
  }
  
  // 计算月份
  while (days >= daysInMonth(dt.month, dt.year)) {
    days -= daysInMonth(dt.month, dt.year);
    dt.month++;
    
    if (dt.month > 12) {
      dt.month = 1;
      dt.year++;
    }
  }
  
  // 计算日期
  dt.day += days;
  
  // 调整年份为两位数格式（与原代码兼容）
  dt.year = dt.year % 100;
  
  return dt;
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
    DynamicJsonDocument doc(4096); // 根据实际需要调整大小
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
    currentWeather.WindSpeed = formatWindSpeed(lives["windpower"].as<String>());
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
    // 天气更新成功，刷新显示
    showTimeDisplay();
  }
  lastWeatherUpdate = millis();
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
// 将中文风向转换为英文
String translateWindDirection(const String& chineseDirection) {
  if (chineseDirection == "东") return "East";
  if (chineseDirection == "西") return "West";
  if (chineseDirection == "南") return "South";
  if (chineseDirection == "北") return "North";
  if (chineseDirection == "东北") return "Northeast";
  if (chineseDirection == "西北") return "Northwest";
  if (chineseDirection == "东南") return "Southeast";
  if (chineseDirection == "西南") return "Southwest";
  return chineseDirection; // 如果没有匹配的，返回原始值
}

// 格式化风速字符串，将≤替换成<=，≥替换成>=
String formatWindSpeed(const String& windSpeed) {
  String formatted = windSpeed;
  formatted.replace("≤", "<=");
  formatted.replace("≥", ">=");
  return formatted;
}

// BM8563 RTC 实现函数

// 十进制转BCD
uint8_t decToBcd(uint8_t val) {
  return ((val / 10) << 4) + (val % 10);
}

// BCD转十进制
uint8_t bcdToDec(uint8_t val) {
  return ((val >> 4) * 10) + (val & 0x0F);
}

// 初始化BM8563
void initBM8563() {
  Serial.println("Initializing BM8563 RTC...");
  
  // 检查BM8563是否存在
  Wire.beginTransmission(BM8563_I2C_ADDRESS);
  if (Wire.endTransmission() == 0) {
    Serial.println("BM8563 RTC found and initialized");
  } else {
    Serial.println("BM8563 RTC not found!");
  }
}

// 从BM8563读取时间
bool readTimeFromBM8563() {
  Wire.beginTransmission(BM8563_I2C_ADDRESS);
  Wire.write(BM8563_REG_SECONDS);
  if (Wire.endTransmission() != 0) {
    Serial.println("Failed to communicate with BM8563");
    return false;
  }
  
  Wire.requestFrom(BM8563_I2C_ADDRESS, 7);
  if (Wire.available() < 7) {
    Serial.println("Failed to read time from BM8563");
    return false;
  }
  
  uint8_t seconds = Wire.read() & 0x7F;  // 清除VL位
  uint8_t minutes = Wire.read() & 0x7F;
  uint8_t hours = Wire.read() & 0x3F;    // 清除世纪位
  uint8_t days = Wire.read() & 0x3F;
  uint8_t weekdays = Wire.read() & 0x07;
  uint8_t months = Wire.read() & 0x1F;   // 清除世纪位
  uint8_t years = Wire.read();
  
  // 转换BCD到十进制并更新currentTime
  currentTime.second = bcdToDec(seconds);
  currentTime.minute = bcdToDec(minutes);
  currentTime.hour = bcdToDec(hours);
  currentTime.day = bcdToDec(days);
  currentTime.month = bcdToDec(months);
  currentTime.year = bcdToDec(years);
  
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
}

// 写入时间到BM8563
bool writeTimeToBM8563(const DateTime& dt) {
  Wire.beginTransmission(BM8563_I2C_ADDRESS);
  Wire.write(BM8563_REG_SECONDS);
  Wire.write(decToBcd(dt.second));
  Wire.write(decToBcd(dt.minute));
  Wire.write(decToBcd(dt.hour));
  Wire.write(decToBcd(dt.day));
  Wire.write(0); // weekday (不使用)
  Wire.write(decToBcd(dt.month));
  Wire.write(decToBcd(dt.year));
  
  if (Wire.endTransmission() != 0) {
    Serial.println("Failed to write time to BM8563");
    return false;
  }
  
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
}
