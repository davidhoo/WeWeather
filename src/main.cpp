#include <Arduino.h>
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "SevenSegment42pt7b.h"

// GDEY029T94 2.9寸三色墨水屏 296x128
struct DateTime {
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;
};

DateTime currentTime = {25, 9, 10, 20, 1, 34};
unsigned long lastMillis = 0;

#define EPD_CS    D8
#define EPD_DC    D2
#define EPD_RST   D4
#define EPD_BUSY  D1
GxEPD2_3C<GxEPD2_290_T94, GxEPD2_290_T94::HEIGHT> display(GxEPD2_290_T94(EPD_CS, EPD_DC, EPD_RST, EPD_BUSY));

void showTimeDisplay();
void updateTime();
String getFormattedTime();
String getFormattedDate();
String getLunarDate();
String getDayOfWeek(int year, int month, int day);
void processSerialInput();
void setTimeFromTimestamp(unsigned long timestamp);
DateTime timestampToDateTime(unsigned long timestamp);

void setup() {
  lastMillis = millis();
  display.init();
  display.setRotation(1);
  Serial.begin(115200);
  showTimeDisplay();
}

void loop() {
  static unsigned long lastDisplayUpdate = 0;
  
  // 处理串口输入
  processSerialInput();
  
  updateTime();
  if (millis() - lastDisplayUpdate >= 60000 || lastDisplayUpdate == 0) {
    ESP.wdtFeed();
    showTimeDisplay();
    lastDisplayUpdate = millis();
  }
  
  ESP.wdtFeed();
  delay(1000);
}

void showTimeDisplay() {
  String timeStr = getFormattedTime();
  String dateStr = getFormattedDate();
  String lunarStr = getLunarDate();
  
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    
    // 显示农历（使用小字体，左对齐）
    display.setFont(&FreeMonoBold9pt7b);
    int lunarX = 10; // 左对齐，距离左边缘10像素
    int lunarY = 15; // 顶部位置
    
    display.setCursor(lunarX, lunarY);
    display.print(lunarStr);
    
    // 在农历下方画一条线
    int topLineY = lunarY + 5;
    display.drawLine(10, topLineY, display.width() - 10, topLineY, GxEPD_BLACK);
    
    // 显示时间（使用大字体，居中）
    display.setFont(&SevenSegment42pt7b);
    int16_t tbx, tby;
    uint16_t tbw, tbh;
    display.getTextBounds(timeStr, 0, 0, &tbx, &tby, &tbw, &tbh);
    
    int timeX = (display.width() - tbw) / 2;
    int timeY = topLineY + tbh + 10; // 在上方线下方
    
    display.setCursor(timeX, timeY);
    display.print(timeStr);
    
    // 在时间下方画一条线
    int bottomLineY = timeY + 10;
    display.drawLine(10, bottomLineY, display.width() - 10, bottomLineY, GxEPD_BLACK);
    
    // 显示日期（使用小字体，左对齐）
    display.setFont(&FreeMonoBold9pt7b);
    int dateX = 10; // 左对齐，距离左边缘10像素
    int dateY = bottomLineY + 20; // 线下方25像素
    
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

String getLunarDate() {
  // 简化的农历计算，使用阿拉伯数字格式 M-D
  
  int fullYear = 2000 + currentTime.year;
  int month = currentTime.month;
  int day = currentTime.day;
  
  // 简化计算：基于公历日期估算农历（这是一个简化版本）
  // 2025年9月11日对应农历七月廿一左右
  int lunarMonth = 7; // 七月
  int lunarDay = 21;  // 廿一
  
  // 根据当前日期调整（简化处理）
  int dayOffset = (fullYear - 2025) * 365 + (month - 9) * 30 + (day - 11);
  lunarDay += dayOffset;
  
  // 处理月份和日期溢出
  while (lunarDay > 30) {
    lunarDay -= 30;
    lunarMonth++;
    if (lunarMonth > 12) {
      lunarMonth = 1;
    }
  }
  while (lunarDay < 1) {
    lunarDay += 30;
    lunarMonth--;
    if (lunarMonth < 1) {
      lunarMonth = 12;
    }
  }
  
  // 确保索引在有效范围内
  if (lunarMonth < 1) lunarMonth = 1;
  if (lunarMonth > 12) lunarMonth = 12;
  if (lunarDay < 1) lunarDay = 1;
  if (lunarDay > 30) lunarDay = 30;
  
  // 返回 M-D 格式
  char lunarString[10];
  sprintf(lunarString, "%d-%d", lunarMonth, lunarDay);
  return String(lunarString);
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
  
  // 立即刷新显示
  showTimeDisplay();
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