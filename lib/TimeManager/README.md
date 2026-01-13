# TimeManager 库

时间管理库，集成了 NTP 网络时间同步和 BM8563 实时时钟功能，为 WeWeather 项目提供准确的时间服务。

## 功能特性

- NTP 网络时间同步
- BM8563 RTC 硬件时钟支持
- 自动时间源切换
- 时间有效性检查
- 多种时间格式输出
- 星期几计算
- WiFi 连接状态感知

## 依赖硬件

- BM8563 实时时钟芯片
- WiFi 连接（用于 NTP 同步）

## 使用方法

### 基本初始化

```cpp
#include "TimeManager.h"
#include "BM8563.h"

// 创建 RTC 实例
BM8563 rtc(21, 22);  // SDA=21, SCL=22

// 创建时间管理器实例
TimeManager timeManager(&rtc);

void setup() {
    Serial.begin(115200);
    
    // 初始化时间管理器
    if (!timeManager.begin()) {
        Serial.println("时间管理器初始化失败");
        return;
    }
    
    Serial.println("时间管理器初始化成功");
}
```

### WiFi 连接和时间同步

```cpp
void setupWiFiAndTime() {
    // 连接 WiFi
    WiFi.begin("your_ssid", "your_password");
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("\nWiFi 连接成功");
    
    // 通知时间管理器 WiFi 已连接
    timeManager.setWiFiConnected(true);
    
    // 尝试 NTP 时间同步
    if (timeManager.updateNTPTime()) {
        Serial.println("NTP 时间同步成功");
    } else {
        Serial.println("NTP 时间同步失败，使用 RTC 时间");
    }
}
```

### 获取和显示时间

```cpp
void displayCurrentTime() {
    // 获取当前时间
    DateTime currentTime = timeManager.getCurrentTime();
    
    // 检查时间有效性
    if (timeManager.isTimeValid()) {
        // 使用格式化方法
        String timeStr = TimeManager::getFormattedTime(currentTime);
        String dateStr = TimeManager::getFormattedDate(currentTime);
        String dayOfWeek = TimeManager::getDayOfWeek(
            currentTime.year, currentTime.month, currentTime.day);
        
        Serial.printf("%s %s %s\n", dateStr.c_str(), timeStr.c_str(), dayOfWeek.c_str());
        
        // 或者使用内置格式化方法
        Serial.println(timeManager.getFormattedTimeString());
    } else {
        Serial.println("时间无效");
    }
}
```

### 手动设置时间

```cpp
void setManualTime() {
    DateTime manualTime = {
        .year = 2024,
        .month = 1,
        .day = 15,
        .hour = 14,
        .minute = 30,
        .second = 0
    };
    
    // 设置当前时间
    timeManager.setCurrentTime(manualTime);
    
    // 写入 RTC
    if (timeManager.writeTimeToRTC(manualTime)) {
        Serial.println("时间设置成功并已写入 RTC");
    }
}
```

## API 参考

### 构造函数
- `TimeManager(BM8563* rtc)` - 创建时间管理器实例

### 初始化方法
- `bool begin()` - 初始化时间管理器

### 时间同步方法
- `bool updateNTPTime()` - 执行 NTP 时间同步
- `bool readTimeFromRTC()` - 从 RTC 读取时间
- `bool writeTimeToRTC(const DateTime& dt)` - 写入时间到 RTC

### 时间获取方法
- `DateTime getCurrentTime() const` - 获取当前时间
- `void setCurrentTime(const DateTime& dt)` - 设置当前时间
- `bool isTimeValid() const` - 检查时间有效性

### 格式化方法
- `String getFormattedTimeString() const` - 获取格式化时间字符串
- `static String getFormattedTime(const DateTime& currentTime)` - 格式化时间
- `static String getFormattedDate(const DateTime& currentTime)` - 格式化日期
- `static String getDayOfWeek(int year, int month, int day)` - 获取星期几

### 状态管理
- `void setWiFiConnected(bool connected)` - 设置 WiFi 连接状态

## 数据结构

### DateTime 结构体

```cpp
struct DateTime {
    int year;    // 年份（如 2024）
    int month;   // 月份（1-12）
    int day;     // 日期（1-31）
    int hour;    // 小时（0-23）
    int minute;  // 分钟（0-59）
    int second;  // 秒（0-59）
};
```

## 时间源优先级

1. **NTP 网络时间**（最高优先级）
   - 当 WiFi 连接可用时自动尝试同步
   - 提供最准确的时间信息
   - 支持多个 NTP 服务器冗余

2. **BM8563 RTC 时间**（备用）
   - 在网络不可用时使用
   - 由备用电池供电，断电后保持运行
   - 提供持续的时间服务

## NTP 配置

库内置了多个 NTP 服务器：

- `ntp.aliyun.com`
- `ntp1.aliyun.com`
- `ntp2.aliyun.com`

配置参数：
- 超时时间：10 秒
- 最大重试次数：10 次
- 自动重试机制

## 时间格式示例

### 时间格式
- `getFormattedTime()`: "14:30:25"
- 24 小时制
- HH:MM:SS 格式

### 日期格式
- `getFormattedDate()`: "2024-01-15"
- YYYY-MM-DD 格式

### 星期几
- `getDayOfWeek()`: "Saturday", "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday"
- 英文显示

## 使用场景

### 1. 天气显示系统

```cpp
void updateWeatherDisplay() {
    DateTime now = timeManager.getCurrentTime();
    
    // 在电子墨水屏上显示时间
    display.showTime(now);
    
    // 根据时间更新天气信息
    if (now.hour == 0 && now.minute == 0) {
        // 每天午夜更新天气
        weatherManager.updateWeatherData();
    }
}
```

### 2. 定时任务

```cpp
void checkScheduledTasks() {
    DateTime now = timeManager.getCurrentTime();
    
    // 每小时执行的任务
    if (now.minute == 0 && now.second == 0) {
        performHourlyTask();
    }
    
    // 每天执行的任务
    if (now.hour == 8 && now.minute == 0 && now.second == 0) {
        performDailyTask();
    }
}
```

### 3. 数据记录

```cpp
void logSensorData(float temperature, float humidity) {
    DateTime now = timeManager.getCurrentTime();
    
    String timestamp = timeManager.getFormattedTimeString();
    String datestamp = TimeManager::getFormattedDate(now);
    
    // 记录带时间戳的传感器数据
    Serial.printf("[%s %s] 温度: %.1f°C, 湿度: %.1f%%\n",
                  datestamp.c_str(), timestamp.c_str(), temperature, humidity);
}
```

## 错误处理

库提供了完善的错误处理机制：

1. **NTP 同步失败**：自动回退到 RTC 时间
2. **RTC 读取失败**：使用系统默认时间
3. **时间无效**：提供有效性检查方法
4. **WiFi 断开**：自动切换到 RTC 时间源

## 性能优化

1. **缓存机制**：避免频繁读取 RTC
2. **智能同步**：仅在必要时进行 NTP 同步
3. **低功耗**：RTC 独立运行，减少主控制器负担

## 注意事项

1. **时区设置**：库使用本地时间，需要根据实际时区调整
2. **夏令时**：不支持自动夏令时调整
3. **电池备份**：确保 RTC 有备用电池
4. **网络依赖**：NTP 功能需要稳定的网络连接

## 依赖库

- BM8563：实时时钟驱动
- ESP8266WiFi：WiFi 连接管理
- time：C 标准时间库
- Arduino：基础 Arduino 框架