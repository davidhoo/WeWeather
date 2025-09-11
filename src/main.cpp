#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "SevenSegment42pt7b.h"
#include "Weather_Symbols_Regular9pt7b.h"

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
unsigned long lastPartialRefresh = 0; // 上次局部刷新时间
int lastDisplayedMinute = -1;         // 上次显示的分钟数，用于检测分钟变化
bool isFirstPartialRefresh = true;    // 标记是否为第一次局部刷新

#define EPD_CS    D8
#define EPD_DC    D2
#define EPD_RST   D4
#define EPD_BUSY  D1
GxEPD2_BW<GxEPD2_290_GDEY029T94, GxEPD2_290_GDEY029T94::HEIGHT> display(GxEPD2_290_GDEY029T94(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

void showTimeDisplay();
void showPartialTimeDisplay(); // 局部刷新时钟区域
void updateTime();
String getFormattedTime();
String getFormattedDate();
String getWeatherInfo();
char getWeatherSymbol();
String getDayOfWeek(int year, int month, int day);
void processSerialInput();
void setTimeFromTimestamp(unsigned long timestamp);
DateTime timestampToDateTime(unsigned long timestamp);

void setup() {
  lastMillis = millis();
  lastFullRefresh = millis();
  lastPartialRefresh = millis();
  lastDisplayedMinute = currentTime.minute;
  isFirstPartialRefresh = true;
  display.init();
  display.setRotation(1); // 调整旋转以适应128x296分辨率
  Serial.begin(115200);
  showTimeDisplay();
}
void loop() {
  unsigned long currentMillis = millis();
  
  // 处理串口输入
  processSerialInput();
  // 检查是否需要刷新（分钟变化时）
  bool needRefresh = (currentTime.minute != lastDisplayedMinute);
  
  // 检查是否需要强制全屏刷新（半小时 = 1800000毫秒）
  bool needForceRefresh = (currentMillis - lastFullRefresh >= 1800000);
  
  
  if (needRefresh || needForceRefresh) {
    ESP.wdtFeed();
    if (needForceRefresh) {
      Serial.println("Performing forced full refresh (30min cycle)");
      lastFullRefresh = currentMillis;
      isFirstPartialRefresh = true; // 重置局部刷新标记
      showTimeDisplay();
    } else {
      Serial.println("Performing minute refresh");
      showPartialTimeDisplay(); // 使用局部刷新替代全屏刷新
      isFirstPartialRefresh = false; // 标记已进行过局部刷新
    }
    lastPartialRefresh = currentMillis;
    lastDisplayedMinute = currentTime.minute;
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
    int weatherX = 10; // Left aligned, 10 pixels from left edge
    int weatherY = 15; // Top position
    
    display.setCursor(weatherX, weatherY);
    display.print(weatherStr);
    
    // Display weather symbol (right aligned)
    display.setFont(&Weather_Symbols_Regular9pt7b);
    char symbolStr[2] = {getWeatherSymbol(), '\0'};
    int16_t sbx, sby;
    uint16_t sbw, sbh;
    display.getTextBounds(symbolStr, 0, 0, &sbx, &sby, &sbw, &sbh);
    
    int symbolX = display.width() - sbw - 10; // Right aligned, 10 pixels from right edge
    int symbolY = weatherY; // Same Y position as weather info
    
    display.setCursor(symbolX, symbolY);
    display.print(symbolStr);
    
    // Draw line below weather info
    int topLineY = weatherY + 5;
    display.drawLine(10, topLineY, display.width() - 10, topLineY, GxEPD_BLACK);
    
    // Display time (using large font, centered with fixed position)
    display.setFont(&SevenSegment42pt7b);
    
    // 使用固定的时间字符串"00:00"来计算居中位置，确保位置不变
    String fixedTimeStr = "00:00";
    int16_t tbx, tby;
    uint16_t tbw, tbh;
    display.getTextBounds(fixedTimeStr, 0, 0, &tbx, &tby, &tbw, &tbh);
    
    int timeX = (display.width() - tbw) / 2;
    int timeY = topLineY + tbh + 10; // Below the top line
    
    display.setCursor(timeX, timeY);
    display.print(timeStr);
    
    // Draw line below time
    int bottomLineY = timeY + 10;
    display.drawLine(10, bottomLineY, display.width() - 10, bottomLineY, GxEPD_BLACK);
    
    // Display date (using small font, left aligned)
    display.setFont(&FreeMonoBold9pt7b);
    int dateX = 10; // Left aligned, 10 pixels from left edge
    int dateY = bottomLineY + 20; // 20 pixels below the line
    
    display.setCursor(dateX, dateY);
    display.print(dateStr);
    
  } while (display.nextPage());
  
  display.hibernate();
}

void showPartialTimeDisplay() {
  Serial.println("Starting partial refresh...");
  
  // 如果是第一次局部刷新，需要先进行一次完整的屏幕缓冲区初始化
  if (isFirstPartialRefresh) {
    Serial.println("First partial refresh - initializing screen buffer");
    // 写入完整屏幕缓冲区
    display.writeScreenBuffer(0xFF); // 白色背景
    delay(10);
  }
  
  // 获取完整的时间字符串
  String timeStr = getFormattedTime();
  
  // 使用与showTimeDisplay完全相同的坐标计算（固定位置）
  display.setFont(&SevenSegment42pt7b);
  
  // 使用固定的时间字符串"00:00"来计算居中位置，确保位置不变
  String fixedTimeStr = "00:00";
  int16_t tbx, tby;
  uint16_t tbw, tbh;
  display.getTextBounds(fixedTimeStr, 0, 0, &tbx, &tby, &tbw, &tbh);
  
  int timeX = (display.width() - tbw) / 2;
  int weatherY = 15; // 与showTimeDisplay中的weatherY保持一致
  int topLineY = weatherY + 5;
  int timeY = topLineY + tbh + 10;
  
  // 计算分钟部分的位置（时间字符串的第4和第5个字符）
  // 时间格式为"HH:MM"，分钟部分是最后两个字符
  String hourPart = "00:"; // 前面的部分
  int16_t hourTbx, hourTby;
  uint16_t hourTbw, hourTbh;
  display.getTextBounds(hourPart, 0, 0, &hourTbx, &hourTby, &hourTbw, &hourTbh);
  
  // 分钟部分的起始X坐标
  int minuteX = timeX + hourTbw;
  
  // 计算分钟部分的宽度 - 使用"00"来确保宽度一致
  String minuteTemplate = "00";
  int16_t minuteTbx, minuteTby;
  uint16_t minuteTbw, minuteTbh;
  display.getTextBounds(minuteTemplate, 0, 0, &minuteTbx, &minuteTby, &minuteTbw, &minuteTbh);
  
  // 设置局部窗口，只覆盖分钟部分
  // 确保窗口边界对齐到8像素边界（GDEY029T94的要求）
  int windowX = (minuteX / 8) * 8; // 对齐到8像素边界
  int windowY = timeY - tbh - 2; // 文本的顶部位置
  int windowW = ((minuteTbw + 15) / 8) * 8; // 对齐到8像素边界，增加边距
  int windowH = tbh + 4; // 增加边距
  
  // 确保窗口在屏幕范围内
  if (windowX < 0) windowX = 0;
  if (windowY < 0) windowY = 0;
  if (windowX + windowW > display.width()) windowW = display.width() - windowX;
  if (windowY + windowH > display.height()) windowH = display.height() - windowY;
  
  Serial.printf("Partial window: X=%d, Y=%d, W=%d, H=%d\n", windowX, windowY, windowW, windowH);
  Serial.printf("Minute text position: X=%d, Y=%d\n", minuteX, timeY);
  
  // 使用GxEPD2的标准局部刷新方法
  display.setPartialWindow(windowX, windowY, windowW, windowH);
  display.firstPage();
  do {
    // 清除分钟区域
    display.fillRect(windowX, windowY, windowW, windowH, GxEPD_WHITE);
    
    // 重新绘制分钟部分
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&SevenSegment42pt7b);
    
    // 只绘制分钟部分
    String minuteStr = String(currentTime.minute);
    if (currentTime.minute < 10) {
      minuteStr = "0" + minuteStr;
    }
    display.setCursor(minuteX, timeY);
    display.print(minuteStr);
    
  } while (display.nextPage());
  
  // 添加延时确保刷新完成
  delay(50);
  Serial.println("Partial refresh completed.");
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
          Serial.println("Time synchronized successfully");
        } else {
          Serial.println("Invalid timestamp");
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
  lastPartialRefresh = currentMillis;
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