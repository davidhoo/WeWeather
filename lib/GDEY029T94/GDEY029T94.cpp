#include "GDEY029T94.h"
#include "../WeatherManager/WeatherManager.h"
#include "../LogManager/LogManager.h"

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

void GDEY029T94::showTimeDisplay(const DateTime& currentTime, const WeatherInfo& currentWeather, float temperature, float humidity, float batteryPercentage) {
  String timeStr = TimeManager::getFormattedTime(currentTime);
  String dateStr = TimeManager::getFormattedDate(currentTime);
  String weatherStr = WeatherManager::getWeatherInfo(currentWeather);
  
  // 调试输出移到循环外部，避免重复打印
  LOG_DEBUG_F("Temperature: %.1f, Humidity: %.1f", temperature, humidity);
  
  if (!isnan(temperature) && !isnan(humidity)) {
    LOG_DEBUG("Displaying temperature and humidity...");
    
    // 准备温度和湿度字符串
    char tempStr[16];
    char humStr[16];
    snprintf(tempStr, sizeof(tempStr), "%.0fC", temperature);
    snprintf(humStr, sizeof(humStr), "%.0f%% ", humidity);
    
    LOG_DEBUG_F("Temp string: %s", tempStr);
    LOG_DEBUG_F("Hum string: %s", humStr);
  }
  
  if (!isnan(batteryPercentage)) {
    LOG_DEBUG_F("Battery percentage: %.1f", batteryPercentage);
  }
  
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
    
    int timeX = alignToPixel8((display.width() - tbw) / 2 - 30); // 居中对齐，8像素对齐
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
    
    // 显示温湿度信息（在日期下方，右对齐）
    if (!isnan(temperature) && !isnan(humidity)) {
      display.setFont(&FreeMonoBold9pt7b);
      
      // 准备温度和湿度字符串
      char tempStr[16];
      char humStr[16];
      snprintf(tempStr, sizeof(tempStr), "%.0fC", temperature);
      snprintf(humStr, sizeof(humStr), "%.0f%% ", humidity);
      
      // 计算温度字符串的宽度和位置（右对齐）
      int16_t tbx, tby;
      uint16_t tbw, tbh;
      display.getTextBounds(tempStr, 0, 0, &tbx, &tby, &tbw, &tbh);
      int tempX = alignToPixel8(display.width() - tbw - 10); // 右对齐，8像素对齐
      int tempY = topLineY + 35; // 在线下方40像素处
      
      // 计算湿度字符串的宽度和位置（右对齐）
      int16_t hbx, hby;
      uint16_t hbw, hbh;
      display.getTextBounds(humStr, 0, 0, &hbx, &hby, &hbw, &hbh);
      int humX = alignToPixel8(display.width() - hbw - 10); // 右对齐，8像素对齐
      int humY = tempY + 20; // 在温度下方20像素处
      
      // 显示温度
      display.setCursor(tempX, tempY);
      display.print(tempStr);
      
      // 在温度左侧画一条竖线，连接上下两条线
      int verticalLineX = alignToPixel8(tempX - 10); // 在温度左侧10像素处
      display.drawLine(verticalLineX, topLineY, verticalLineX, bottomLineY, GxEPD_BLACK);
      
      // 显示湿度
      display.setCursor(humX, humY);
      display.print(humStr);
    }
    
    // 显示电池电量（在右下角，与日期同一行）
    if (!isnan(batteryPercentage)) {
      int totalBatteryWidth = 25;
      int batteryX = alignToPixel8(display.width() - totalBatteryWidth);
      int batteryY = dateY;
      drawBatteryIcon(batteryX, batteryY, batteryPercentage);
    }
    
  } while (display.nextPage());
  
  // 在循环外部输出完成信息
  if (!isnan(temperature) && !isnan(humidity)) {
    LOG_DEBUG_F("Temp position: X=%d, Y=55", alignToPixel8(display.width() - 10));
    LOG_DEBUG_F("Hum position: X=%d, Y=75", alignToPixel8(display.width() - 10));
    LOG_DEBUG("Temperature and humidity displayed");
  } else {
    LOG_WARN("Temperature or humidity is NaN, not displaying");
  }
  
  if (!isnan(batteryPercentage)) {
    LOG_DEBUG("Battery icon displayed");
  } else {
    LOG_WARN("Battery percentage is NaN, not displaying");
  }
  
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

void GDEY029T94::drawBatteryIcon(int x, int y, float percentage) {
  int barWidth = 2;
  int barCount = 10;
  int borderThickness = 1;
  int topBottomMargin = 1;
  
  int batteryWidth = barCount * barWidth + 2 * borderThickness + 2;
  int batteryHeight = 12;
  int capWidth = 3;
  int capHeight = 6;
  
  int capX = x - capWidth;
  int capY = y - batteryHeight/2 - capHeight/2 + 2;
  display.fillRect(capX, capY, capWidth, capHeight, GxEPD_BLACK);
  
  display.drawRect(x, y - batteryHeight + 2, batteryWidth, batteryHeight, GxEPD_BLACK);
  
  int filledBars = (int)((percentage / 100.0) * 10 + 0.5);
  if (filledBars > 10) filledBars = 10;
  if (filledBars < 0) filledBars = 0;
  
  int barY = y - batteryHeight + 2 + topBottomMargin + 1;
  int barHeight = batteryHeight - 2 * borderThickness - 2 * topBottomMargin;
  
  int rightX = x + batteryWidth - borderThickness -1;
  
  for (int i = 0; i < barCount; i++) {
    int barX = rightX - (i + 1) * barWidth;
    
    if (i < filledBars) {
      display.fillRect(barX, barY, barWidth, barHeight, GxEPD_BLACK);
    } else {
      display.fillRect(barX, barY, barWidth, barHeight, GxEPD_WHITE);
    }
  }
}

void GDEY029T94::showConfigDisplay(const char* apName, const char* apIP) {
  LOG_INFO("Displaying configuration mode screen...");
  
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);
    
    int y = 20;
    int lineHeight = 20;
    
    // 显示标题
    display.setCursor(alignToPixel8(10), y);
    display.print("Config Mode");
    y += lineHeight;
    
    // 画分隔线
    display.drawLine(alignToPixel8(10), y, display.width() - alignToPixel8(10), y, GxEPD_BLACK);
    y += lineHeight;
    
    // 显示WiFi信息（WIFI: 和名称在同一行）
    display.setCursor(alignToPixel8(10), y);
    display.print("WIFI: ");
    display.print(apName);
    y += lineHeight;
    
    // 显示IP信息（IP: 和地址在同一行）
    display.setCursor(alignToPixel8(10), y);
    display.print("IP: ");
    display.print(apIP);
    y += lineHeight;
    
    // 画分隔线
    display.drawLine(alignToPixel8(10), y, display.width() - alignToPixel8(10), y, GxEPD_BLACK);
    y += lineHeight;
    
    // 显示连接提示（不换行）
    display.setCursor(alignToPixel8(10), y);
    display.print("Connect wifi and browse IP");
    
  } while (display.nextPage());
  
  display.hibernate();
  LOG_INFO("Configuration mode screen displayed");
}
