#include "GDEY029T94.h"
#include "../WeatherManager/WeatherManager.h"

GDEY029T94::GDEY029T94(uint8_t cs, uint8_t dc, uint8_t rst, uint8_t busy)
  : display(GxEPD2_290_GDEY029T94(cs, dc, rst, busy)), 
    timeFont(nullptr), 
    weatherSymbolFont(nullptr) {
}

void GDEY029T94::begin() {
  display.init();
}

void GDEY029T94::setRotation(int rotation) {
  display.setRotation(rotation);
}

void GDEY029T94::showTimeDisplay(const DateTime& currentTime, const WeatherInfo& currentWeather) {
  String timeStr = getFormattedTime(currentTime);
  String dateStr = getFormattedDate(currentTime);
  String weatherStr = WeatherManager::getWeatherInfo(currentWeather);
  
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    
    // 显示天气信息（使用小字体，左对齐）
    display.setFont(&FreeMonoBold9pt7b);
    int weatherX = alignToPixel8(10); // 左对齐，从左边缘8像素对齐
    int weatherY = 15; // 顶部位置
    
    display.setCursor(weatherX, weatherY);
    display.print(weatherStr);
    
    // 显示天气符号（右对齐）
    if (weatherSymbolFont) {
      display.setFont(weatherSymbolFont);
    } else {
      display.setFont(&FreeMonoBold9pt7b);
    }
    
    char symbolStr[2] = {WeatherManager::getWeatherSymbol(currentWeather), '\0'};
    int16_t sbx, sby;
    uint16_t sbw, sbh;
    display.getTextBounds(symbolStr, 0, 0, &sbx, &sby, &sbw, &sbh);
    
    int symbolX = alignToPixel8(display.width() - sbw - 10); // 右对齐，8像素对齐
    int symbolY = weatherY; // 与天气信息相同的Y位置
    
    display.setCursor(symbolX, symbolY);
    display.print(symbolStr);
    
    // 在天气信息下方画线
    int topLineY = weatherY + 5;
    display.drawLine(alignToPixel8(10), topLineY, display.width() - alignToPixel8(10), topLineY, GxEPD_BLACK);
    
    // 显示时间（使用大字体，居中显示）
    if (timeFont) {
      display.setFont(timeFont);
    } else {
      display.setFont(&FreeMonoBold9pt7b);
    }
    
    // 使用固定的时间字符串"00:00"来计算居中位置，确保位置不变
    String fixedTimeStr = "00:00";
    int16_t tbx, tby;
    uint16_t tbw, tbh;
    display.getTextBounds(fixedTimeStr, 0, 0, &tbx, &tby, &tbw, &tbh);
    
    int timeX = alignToPixel8((display.width() - tbw) / 2); // 居中对齐，8像素对齐
    int timeY = topLineY + tbh + 10; // 在顶部线下方（减少间距）
    
    display.setCursor(timeX, timeY);
    display.print(timeStr);
    
    // 在时间下方画线
    int bottomLineY = timeY + 10;
    display.drawLine(alignToPixel8(10), bottomLineY, display.width() - alignToPixel8(10), bottomLineY, GxEPD_BLACK);
    
    // 显示日期（使用小字体，左对齐）
    display.setFont(&FreeMonoBold9pt7b);
    int dateX = alignToPixel8(10); // 左对齐，从左边缘8像素对齐
    int dateY = bottomLineY + 20; // 在线下方20像素处（减少间距）
    
    display.setCursor(dateX, dateY);
    display.print(dateStr);
    
  } while (display.nextPage());
  
  display.hibernate();
}

void GDEY029T94::setTimeFont(const GFXfont* font) {
  timeFont = font;
}

void GDEY029T94::setWeatherSymbolFont(const GFXfont* font) {
  weatherSymbolFont = font;
}

int GDEY029T94::alignToPixel8(int x) {
  return (x / 8) * 8;
}

String GDEY029T94::getFormattedTime(const DateTime& currentTime) {
  char timeString[6];
  sprintf(timeString, "%02d:%02d", currentTime.hour, currentTime.minute);
  
  return String(timeString);
}

String GDEY029T94::getFormattedDate(const DateTime& currentTime) {
  // 获取完整年份（2000 + 两位数年份）
  int fullYear = 2000 + currentTime.year;
  
  // 获取星期几
  String dayOfWeek = getDayOfWeek(fullYear, currentTime.month, currentTime.day);
  
  char dateString[50];
  sprintf(dateString, "%04d/%02d/%02d %s", fullYear, currentTime.month, currentTime.day, dayOfWeek.c_str());
  
  return String(dateString);
}

String GDEY029T94::getDayOfWeek(int year, int month, int day) {
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
