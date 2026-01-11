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

void GDEY029T94::showTimeDisplay(const DateTime& currentTime, const WeatherInfo& currentWeather, float temperature, float humidity, float batteryPercentage) {
  String timeStr = TimeManager::getFormattedTime(currentTime);
  String dateStr = TimeManager::getFormattedDate(currentTime);
  String weatherStr = WeatherManager::getWeatherInfo(currentWeather);
  
  // 调试输出移到循环外部，避免重复打印
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print(", Humidity: ");
  Serial.println(humidity);
  
  if (!isnan(temperature) && !isnan(humidity)) {
    Serial.println("Displaying temperature and humidity...");
    
    // 准备温度和湿度字符串
    char tempStr[16];
    char humStr[16];
    snprintf(tempStr, sizeof(tempStr), "%.0fC", temperature);
    snprintf(humStr, sizeof(humStr), "%.0f%% ", humidity);
    
    Serial.print("Temp string: ");
    Serial.println(tempStr);
    Serial.print("Hum string: ");
    Serial.println(humStr);
  }
  
  if (!isnan(batteryPercentage)) {
    Serial.print("Battery percentage: ");
    Serial.println(batteryPercentage);
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
      // 计算电池图标位置（右对齐，与日期同一行）
      // 电池总宽度 = 22像素（电池主体）+ 3像素（正极帽）= 25像素
      int totalBatteryWidth = 25;
      int batteryX = alignToPixel8(display.width() - totalBatteryWidth); // 右对齐，距离右边缘10像素
      int batteryY = dateY; // 与日期相同的Y位置
      
      drawBatteryIcon(batteryX, batteryY, batteryPercentage);
    }
    
  } while (display.nextPage());
  
  // 在循环外部输出完成信息
  if (!isnan(temperature) && !isnan(humidity)) {
    Serial.print("Temp position: X=");
    Serial.print(alignToPixel8(display.width() - 10));
    Serial.print(", Y=55");
    Serial.print("Hum position: X=");
    Serial.print(alignToPixel8(display.width() - 10));
    Serial.println(", Y=75");
    Serial.println("Temperature and humidity displayed");
  } else {
    Serial.println("Temperature or humidity is NaN, not displaying");
  }
  
  if (!isnan(batteryPercentage)) {
    Serial.println("Battery icon displayed");
  } else {
    Serial.println("Battery percentage is NaN, not displaying");
  }
  
  display.hibernate();
}

void GDEY029T94::showWebConfigInfo(const String& ssid, const String& ip) {
  Serial.println("显示 Web 配置信息到屏幕...");
  
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeMonoBold9pt7b);
    
    // 标题
    int titleY = 20;
    display.setCursor(alignToPixel8(10), titleY);
    display.print("Web Config Mode");
    
    // 分隔线
    int lineY = titleY + 10;
    display.drawLine(alignToPixel8(10), lineY, display.width() - alignToPixel8(10), lineY, GxEPD_BLACK);
    
    // SSID 信息（同一行显示）
    int ssidY = lineY + 30;
    display.setCursor(alignToPixel8(10), ssidY);
    display.print("SSID: ");
    display.print(ssid);
    
    // IP 信息（同一行显示）
    int ipY = ssidY + 25;
    display.setCursor(alignToPixel8(10), ipY);
    display.print("IP: ");
    display.print(ip);
    
    // 访问提示（简化为一行）
    int urlY = ipY + 35;
    display.setCursor(alignToPixel8(10), urlY);
    display.print("Visit: http://");
    display.print(ip);
    
  } while (display.nextPage());
  
  Serial.println("Web 配置信息已显示到屏幕");
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
  // 电池设计参数
  int barWidth = 2;           // 每格电量用2像素宽
  int barCount = 10;          // 总共10格
  int borderThickness = 1;    // 边框厚度
  int topBottomMargin = 1;    // 上下边距
  
  // 计算电池尺寸
  int batteryWidth = barCount * barWidth + 2 * borderThickness + 2; // 10格*2像素 + 左右边框
  int batteryHeight = 12;     // 保持高度12像素
  int capWidth = 3;
  int capHeight = 6;
  
  // 绘制电池正极帽（左边）
  int capX = x - capWidth;
  int capY = y - batteryHeight/2 - capHeight/2 + 2;
  display.fillRect(capX, capY, capWidth, capHeight, GxEPD_BLACK);
  
  // 绘制电池外框
  display.drawRect(x, y - batteryHeight + 2, batteryWidth, batteryHeight, GxEPD_BLACK);
  
  // 计算需要填充的格数（0-10格）
  int filledBars = (int)((percentage / 100.0) * 10 + 0.5); // 四舍五入
  if (filledBars > 10) filledBars = 10;
  if (filledBars < 0) filledBars = 0;
  
  // 绘制10格电量，每格2像素宽，从右往左排列
  int barY = y - batteryHeight + 2 + topBottomMargin + 1; // 上边框 + 上边距
  int barHeight = batteryHeight - 2 * borderThickness - 2 * topBottomMargin; // 减去上下边框和边距
  
  // 从右边开始绘制
  int rightX = x + batteryWidth - borderThickness -1; // 右边界（减去右边框）
  
  for (int i = 0; i < barCount; i++) {
    int barX = rightX - (i + 1) * barWidth; // 从右往左，每格2像素宽
    
    if (i < filledBars) {
      // 有电量的格子（黑色），从右边开始
      display.fillRect(barX, barY, barWidth, barHeight, GxEPD_BLACK);
    } else {
      // 无电量的格子（白色）
      display.fillRect(barX, barY, barWidth, barHeight, GxEPD_WHITE);
    }
  }
}

