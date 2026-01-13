# WeatherManager 库

天气管理库，提供天气数据获取、缓存和显示功能，支持高德地图天气 API，集成网络请求和本地存储。

## 功能特性

- 高德地图天气 API 集成
- 天气数据缓存机制
- 自动更新间隔控制
- 天气符号映射
- 风向和风速处理
- EEPROM 数据持久化
- 网络状态感知
- 多种天气信息格式化

## 天气符号映射

| 符号 | 天气状况 | 英文描述 |
|------|----------|----------|
| n    | 晴       | Sunny    |
| d    | 雪       | Snow     |
| m    | 雨       | Rain     |
| l    | 雾       | Fog      |
| c    | 阴       | Overcast |
| o    | 多云     | Cloudy   |
| k    | 雷雨     | Thunderstorm |
| a    | 雷       | Thunder  |
| p    | 少云     | Partly Cloudy |
| f    | 风       | Wind     |
| e    | 冷       | Cold     |
| h    | 热       | Hot      |

## 使用方法

### 基本初始化

```cpp
#include "WeatherManager.h"
#include "BM8563.h"

// 创建 RTC 实例
BM8563 rtc(21, 22);

// 创建天气管理器实例
WeatherManager weatherManager("your_api_key", "110000", &rtc, 512);

void setup() {
    Serial.begin(115200);
    
    // 初始化天气管理器
    weatherManager.begin();
    
    // 设置更新间隔（秒）
    weatherManager.setUpdateInterval(1800);  // 30分钟
}
```

### 获取和更新天气信息

```cpp
void updateAndDisplayWeather() {
    // 更新天气信息（自动判断是否需要网络更新）
    if (weatherManager.updateWeather()) {
        Serial.println("天气信息更新成功");
        
        // 获取当前天气
        WeatherInfo currentWeather = weatherManager.getCurrentWeather();
        
        // 显示天气信息
        displayWeatherInfo(currentWeather);
    } else {
        Serial.println("天气信息更新失败，使用缓存数据");
        
        // 获取缓存的天气信息
        WeatherInfo cachedWeather = weatherManager.getCurrentWeather();
        displayWeatherInfo(cachedWeather);
    }
}

void displayWeatherInfo(const WeatherInfo& weather) {
    Serial.printf("温度: %.1f°C\n", weather.Temperature);
    Serial.printf("湿度: %d%%\n", weather.Humidity);
    Serial.printf("天气: %s (%c)\n", weather.Weather.c_str(), weather.Symbol);
    Serial.printf("风向: %s\n", weather.WindDirection.c_str());
    Serial.printf("风速: %s\n", weather.WindSpeed.c_str());
    
    // 获取格式化的天气信息字符串
    String weatherStr = WeatherManager::getWeatherInfo(weather);
    Serial.printf("完整信息: %s\n", weatherStr.c_str());
}
```

### 强制网络更新

```cpp
void forceWeatherUpdate() {
    Serial.println("强制从网络更新天气...");
    
    if (weatherManager.fetchWeatherFromNetwork()) {
        Serial.println("网络天气更新成功");
        
        // 保存到存储
        weatherManager.writeWeatherToStorage();
        
        // 显示更新时间
        unsigned long updateTime = weatherManager.getLastUpdateTime();
        Serial.printf("更新时间: %lu\n", updateTime);
    } else {
        Serial.println("网络天气更新失败");
    }
}
```

### 天气数据处理

```cpp
void processWeatherData() {
    WeatherInfo weather = weatherManager.getCurrentWeather();
    
    // 获取天气符号
    char symbol = WeatherManager::getWeatherSymbol(weather);
    Serial.printf("天气符号: %c\n", symbol);
    
    // 映射天气状况到符号
    char mappedSymbol = WeatherManager::mapWeatherToSymbol("晴");
    Serial.printf("映射符号: %c\n", mappedSymbol);
    
    // 测试其他天气状况
    char snowSymbol = WeatherManager::mapWeatherToSymbol("雪");
    char rainSymbol = WeatherManager::mapWeatherToSymbol("雷雨");
    Serial.printf("雪符号: %c, 雷雨符号: %c\n", snowSymbol, rainSymbol);
    
    // 转换风向
    String englishWind = WeatherManager::translateWindDirection("东北风");
    Serial.printf("风向翻译: %s\n", englishWind.c_str());
    
    // 格式化风速
    String formattedSpeed = WeatherManager::formatWindSpeed("3-4级");
    Serial.printf("风速格式化: %s\n", formattedSpeed.c_str());
}
```

## API 参考

### 构造函数
- `WeatherManager(const char* apiKey, const String& cityCode, BM8563* rtc, int eepromSize = 512)` - 创建天气管理器实例

### 初始化方法
- `void begin()` - 初始化天气管理器

### 天气数据获取
- `WeatherInfo getCurrentWeather()` - 获取当前天气信息
- `bool updateWeather(bool forceUpdate = false)` - 更新天气信息
- `bool shouldUpdateFromNetwork()` - 判断是否需要网络更新
- `bool fetchWeatherFromNetwork()` - 从网络获取天气数据
- `bool readWeatherFromStorage()` - 从存储读取天气信息

### 存储管理
- `bool writeWeatherToStorage()` - 将天气信息写入存储
- `void clearWeatherData()` - 清除存储中的天气数据

### 更新控制
- `void setUpdateInterval(unsigned long intervalSeconds)` - 设置更新间隔
- `unsigned long getLastUpdateTime()` - 获取上次更新时间
- `bool setUpdateTime(unsigned long timestamp)` - 设置更新时间戳

### 静态工具方法
- `static char mapWeatherToSymbol(const String& weather)` - 天气状况映射到符号
- `static String translateWindDirection(const String& chineseDirection)` - 中文风向转英文
- `static String formatWindSpeed(const String& windSpeed)` - 格式化风速
- `static String getWeatherInfo(const WeatherInfo& currentWeather)` - 获取天气信息字符串
- `static char getWeatherSymbol(const WeatherInfo& currentWeather)` - 获取天气符号

## 数据结构

### WeatherInfo 结构体

```cpp
struct WeatherInfo {
    float Temperature;    // 温度（摄氏度）
    int Humidity;         // 湿度百分比
    char Symbol;          // 天气符号字符
    String WindDirection; // 风向
    String WindSpeed;     // 风速
    String Weather;       // 天气状况
};
```

## 更新策略

### 自动更新逻辑

1. **首次启动**：检查存储，无数据则强制网络更新
2. **正常更新**：检查时间间隔，超时则网络更新
3. **网络失败**：使用缓存数据
4. **强制更新**：忽略时间间隔，直接网络请求

### 更新间隔

- 默认间隔：由 `WEATHER_UPDATE_INTERVAL` 宏定义（在 config.h 中）
- 建议间隔：15-60分钟
- 可通过 `setUpdateInterval()` 方法动态调整

## 使用场景

### 1. 定时天气显示

```cpp
void setup() {
    weatherManager.begin();
    weatherManager.setUpdateInterval(1800);  // 30分钟更新
}

void loop() {
    static unsigned long lastDisplay = 0;
    
    if (millis() - lastDisplay > 60000) {  // 每分钟显示
        WeatherInfo weather = weatherManager.getCurrentWeather();
        displayOnScreen(weather);
        lastDisplay = millis();
    }
    
    // 检查是否需要更新
    if (weatherManager.shouldUpdateFromNetwork()) {
        weatherManager.updateWeather();
    }
    
    delay(1000);
}
```

### 2. 智能更新策略

```cpp
void smartWeatherUpdate() {
    // 检查网络连接
    if (WiFi.status() == WL_CONNECTED) {
        // 检查是否在夜间（减少更新频率）
        time_t now = time(nullptr);
        struct tm* timeinfo = localtime(&now);
        int hour = timeinfo->tm_hour;
        
        if (hour >= 0 && hour < 6) {
            // 夜间：1小时更新一次
            weatherManager.setUpdateInterval(3600);
        } else {
            // 白天：30分钟更新一次
            weatherManager.setUpdateInterval(1800);
        }
        
        weatherManager.updateWeather();
    }
}
```

### 3. 天气预警

```cpp
void checkWeatherAlerts() {
    WeatherInfo weather = weatherManager.getCurrentWeather();
    
    // 高温预警
    if (weather.Temperature > 35.0) {
        Serial.println("高温预警：温度超过35°C");
    }
    
    // 低温预警
    if (weather.Temperature < -10.0) {
        Serial.println("低温预警：温度低于-10°C");
    }
    
    // 恶劣天气预警
    if (weather.Symbol == 'd' || weather.Symbol == 'k') {
        Serial.println("恶劣天气预警：" + weather.Weather);
    }
}
```

## 错误处理

库提供了完善的错误处理机制：

1. **网络请求失败**：自动使用缓存数据
2. **API 解析错误**：保持现有数据不变
3. **存储读写失败**：使用内存中的数据
4. **时间戳无效**：强制网络更新

## 性能优化

1. **智能缓存**：避免频繁网络请求
2. **增量更新**：仅在数据变化时更新显示
3. **内存管理**：合理使用字符串和缓冲区
4. **网络优化**：使用 HTTP Keep-Alive

## 注意事项

1. **API 密钥**：需要有效的高德地图天气 API 密钥
2. **网络依赖**：天气更新需要稳定的网络连接
3. **城市代码**：使用正确的城市代码格式
4. **更新频率**：避免过于频繁的 API 调用
5. **数据有效期**：天气数据建议不超过 2 小时

## 依赖库

- ESP8266WiFi：WiFi 连接管理
- ESP8266HTTPClient：HTTP 请求
- WiFiClientSecure：HTTPS 连接
- ArduinoJson：JSON 数据解析
- BM8563：实时时钟
- ConfigManager：配置数据管理

## API 配置

### 高德地图天气 API

- API 地址：`https://restapi.amap.com/v3/weather/weatherInfo`
- 请求参数：
  - `key`: API 密钥
  - `city`: 城市代码
  - `extensions`: 扩展信息（base）
  - `output`: 输出格式（JSON）
- 超时设置：5秒
- SSL 验证：跳过证书验证（setInsecure）

### 城市代码格式

- 北京市：110000
- 上海市：310000
- 广州市：440100
- 深圳市：440300

## 示例项目

### 完整天气站

```cpp
#include "WeatherManager.h"
#include "GDEY029T94.h"

WeatherManager weatherManager("your_api_key", "110000", &rtc);
GDEY029T94 display(5, 4, 16, 2);

void setup() {
    Serial.begin(115200);
    
    // 连接 WiFi
    connectWiFi();
    
    // 初始化组件
    weatherManager.begin();
    display.begin();
    
    // 首次更新
    weatherManager.updateWeather();
}

void loop() {
    static unsigned long lastUpdate = 0;
    
    // 每小时更新一次天气
    if (millis() - lastUpdate > 3600000) {
        weatherManager.updateWeather();
        lastUpdate = millis();
    }
    
    // 获取当前时间和天气
    DateTime now = timeManager.getCurrentTime();
    WeatherInfo weather = weatherManager.getCurrentWeather();
    
    // 更新显示
    display.showTimeDisplay(now, weather);
    
    delay(60000);  // 每分钟刷新显示
}