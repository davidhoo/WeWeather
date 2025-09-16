#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <time.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "Weather_Symbols_Regular9pt7b.h"
#include "DSEG7Modern_Bold28pt7b.h"

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
  float temperature;    // Temperature in Celsius
  int humidity;         // Humidity percentage
  char symbol;          // Weather symbol character
  // Weather symbol mapping:
  // n=晴(sunny), d=雪(snow), m=雨(rain), l=雾(fog), c=阴(overcast), o=多云(cloudy), k=雷雨(thunderstorm)
};

DateTime currentTime = {25, 9, 10, 20, 1, 34};
WeatherInfo currentWeather = {23.5, 65, 'n'}; // 23.5°C, 65% humidity, sunny
unsigned long lastMillis = 0;
unsigned long lastFullRefresh = 0;    // 上次全屏刷新时间
int lastDisplayedMinute = -1;         // 上次显示的分钟数，用于检测分钟变化
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
#define EPD_RST   D4
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

// 8像素对齐辅助函数 - SSD1680控制器要求x坐标必须8像素对齐
int alignToPixel8(int x) {
  return (x / 8) * 8;
}

void setup() {
  lastMillis = millis();
  lastFullRefresh = millis();
  lastDisplayedMinute = currentTime.minute;
  display.init();
  
  Serial.begin(115200);
  
  // 初始化WiFi连接
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  connectToWiFi();
  
  display.setRotation(1); // 调整旋转以适应128x296分辨率
  
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
    // 刷新屏幕前更新NTP时间
    updateNTPTime();
    showTimeDisplay();
    lastDisplayedMinute = currentTime.minute;
    lastFullRefresh = currentMillis;
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
  // Format weather information for display (temperature and humidity only)
  char weatherString[20];
  sprintf(weatherString, "%.1fC %d%%",
          currentWeather.temperature,
          currentWeather.humidity);
  
  return String(weatherString);
}

char getWeatherSymbol() {
  // Return weather symbol character
  return currentWeather.symbol;
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
  currentTime.year = (timeinfo->tm_year + 1900) % 100;  // 转换为两位数年份
  currentTime.month = timeinfo->tm_mon + 1;            // tm_mon是从0开始的
  currentTime.day = timeinfo->tm_mday;
  currentTime.hour = timeinfo->tm_hour;
  currentTime.minute = timeinfo->tm_min;
  currentTime.second = timeinfo->tm_sec;
  
  Serial.print("Time updated: ");
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
