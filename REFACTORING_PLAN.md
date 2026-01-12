# WeWeather 项目重构与优化方案

## 📊 项目现状分析

### 项目概况
- **项目类型**: ESP8266 物联网天气显示终端
- **核心功能**: 天气显示、时间管理、温湿度监测、电池监控、WiFi 配网
- **硬件平台**: ESP8266 + 2.9寸墨水屏 + BM8563 RTC + SHT40 传感器
- **代码规模**: 约 2000+ 行代码，7个自定义库模块

### 架构优势 ✅

1. **模块化设计清晰**
   - WiFiManager、WeatherManager、TimeManager 等功能模块独立
   - 每个库都有独立的头文件和实现文件
   - 职责分离合理

2. **硬件抽象良好**
   - RTC、传感器、显示屏都有独立的驱动封装
   - I2C 和 SPI 设备管理清晰

3. **功耗优化到位**
   - 深度睡眠机制完善
   - RTC 定时器唤醒
   - 智能缓存减少网络请求

## 🔍 存在的核心问题

### 1. [`main.cpp`](src/main.cpp) 代码组织问题 ⚠️

**问题描述**:
- [`setup()`](src/main.cpp:58) 函数过长（118行），包含太多逻辑
- 初始化、WiFi连接、数据采集、显示等多个职责混在一起
- 代码可读性和可维护性差

**当前代码结构**:
```cpp
void setup() {
    // 1. 串口初始化
    // 2. 各种硬件初始化（60行）
    // 3. WiFi连接逻辑（40行）
    // 4. 数据采集（15行）
    // 5. 显示和睡眠（3行）
}
```

**建议优化**:
```cpp
void setup() {
    initializeSerial();
    initializeHardware();
    connectWiFi();
    updateAndDisplay();
    goToDeepSleep();
}
```

### 2. 重复代码 ⚠️

**问题1: RTC 中断清除重复**

在 [`main.cpp:84-90`](src/main.cpp:84) 和 [`main.cpp:189-197`](src/main.cpp:189) 中重复：
```cpp
// 第一次：setup() 中
rtc.clearTimerFlag();
rtc.clearAlarmFlag();
rtc.enableTimerInterrupt(false);
rtc.enableAlarmInterrupt(false);

// 第二次：goToDeepSleep() 中
rtc.clearTimerFlag();
rtc.clearAlarmFlag();
// ... 然后设置定时器
```

**建议**: 在 [`BM8563`](lib/BM8563/BM8563.h) 类中添加辅助方法：
```cpp
// lib/BM8563/BM8563.h
void resetInterrupts() {
    clearTimerFlag();
    clearAlarmFlag();
    enableTimerInterrupt(false);
    enableAlarmInterrupt(false);
}
```

### 3. 魔法数字和硬编码 ⚠️

**问题描述**:
```cpp
#define DEEP_SLEEP_DURATION 60  // 定义了但未使用
Serial.begin(74880);            // 特殊波特率无注释
rtc.setTimer(60, BM8563_TIMER_1HZ);  // 硬编码的60
```

**建议**: 统一使用常量并添加注释：
```cpp
// 深度睡眠配置
constexpr uint16_t DEEP_SLEEP_SECONDS = 60;  // 1分钟唤醒一次

// ESP8266 默认调试波特率（与 ROM bootloader 一致）
constexpr uint32_t SERIAL_BAUD_RATE = 74880;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    // ...
}

void goToDeepSleep() {
    rtc.setTimer(DEEP_SLEEP_SECONDS, BM8563_TIMER_1HZ);
    // ...
}
```

### 4. 错误处理不一致 ⚠️

**问题描述**:
- 有些函数返回 bool 但未检查
- 有些直接 Serial.println 错误信息
- 传感器读取失败后的处理不统一

**示例**:
```cpp
// 当前代码
if (sht40.begin()) {
    Serial.println("SHT40 initialized successfully");
} else {
    Serial.println("Failed to initialize SHT40");
    // 然后呢？继续运行还是重启？
}

// 后续使用时
if (sht40.readTemperatureHumidity(temperature, humidity)) {
    // 成功
} else {
    Serial.println("Failed to read SHT40 sensor");
    temperature = NAN;  // 使用 NAN 作为默认值
    humidity = NAN;
}
```

**建议**: 统一错误处理策略：
```cpp
// 定义错误处理策略
enum class InitResult {
    SUCCESS,
    FAILED_CRITICAL,    // 关键组件失败，需要重启
    FAILED_NON_CRITICAL // 非关键组件失败，可以继续
};

InitResult initializeSensors() {
    if (!sht40.begin()) {
        Serial.println("Warning: SHT40 init failed, will use NAN values");
        return InitResult::FAILED_NON_CRITICAL;
    }
    return InitResult::SUCCESS;
}
```

### 5. 配置管理分散 ⚠️

**问题描述**:
- 引脚定义在 [`main.cpp`](src/main.cpp:21) 中
- API 配置在 [`config.h`](config.h) 中
- 深度睡眠时间在 [`main.cpp`](src/main.cpp:18) 中
- 修改配置需要改多个文件

**建议**: 集中配置管理：
```cpp
// config.h - 统一配置文件
#ifndef CONFIG_H
#define CONFIG_H

// ========== 硬件引脚配置 ==========
// I2C 引脚 (BM8563 & SHT40)
#define I2C_SDA_PIN 2   // GPIO-2 (D4)
#define I2C_SCL_PIN 12  // GPIO-12 (D6)

// 墨水屏 SPI 引脚 (GDEY029T94)
#define EPD_CS_PIN   D8
#define EPD_DC_PIN   D2
#define EPD_RST_PIN  D0
#define EPD_BUSY_PIN D1

// ========== 电源管理配置 ==========
#define DEEP_SLEEP_SECONDS 60  // 深度睡眠时间（秒）
#define RTC_TIMER_SECONDS  60  // RTC 定时器时间（应与深度睡眠一致）

// ========== 串口配置 ==========
#define SERIAL_BAUD_RATE 74880  // ESP8266 默认波特率

// ========== API 配置 ==========
#define AMAP_API_KEY "your_amap_api_key_here"
#define CITY_CODE "110108"

// ========== WiFi 配置 ==========
#define DEFAULT_WIFI_SSID "your_wifi_ssid"
#define DEFAULT_WIFI_PASSWORD "your_wifi_password"
#define DEFAULT_MAC_ADDRESS "AA:BB:CC:DD:EE:FF"

// ========== 天气更新配置 ==========
#define WEATHER_UPDATE_INTERVAL 1800  // 30分钟（秒）

#endif // CONFIG_H
```

### 6. 内存使用可优化 ⚠️

**问题描述**:
- 大量使用 String 类（动态内存分配）
- 可能导致内存碎片
- ESP8266 内存有限（80KB）

**示例**:
```cpp
// 当前代码
String cityCode = CITY_CODE;  // 动态分配
String getLocalIP();          // 返回 String

// WiFiManager.h
String getScannedSSID(int index);
String getStatusString();
```

**建议**: 关键路径使用固定缓冲区：
```cpp
// 使用 const char* 或 char[]
const char* cityCode = CITY_CODE;

// 或使用固定缓冲区
char ipBuffer[16];
void getLocalIP(char* buffer, size_t size);

// 非关键路径可以继续使用 String（如配网界面）
```

### 7. 代码注释不足 ⚠️

**问题描述**:
- 关键逻辑缺乏注释
- 特殊处理没有说明原因
- 不利于后续维护

**示例**:
```cpp
// 当前代码
Serial.begin(74880);  // 为什么是这个波特率？

rtc.clearTimerFlag();
rtc.clearAlarmFlag();  // 为什么要清除？

epd.setRotation(1);  // 为什么是1？
```

**建议**: 添加必要注释：
```cpp
// ESP8266 ROM bootloader 使用 74880 波特率，保持一致便于调试
Serial.begin(74880);

// 清除 RTC 中断标志，防止 INT 引脚持续拉低导致无法进入深度睡眠
rtc.clearTimerFlag();
rtc.clearAlarmFlag();

// 旋转90度以适应 128x296 分辨率的横向显示
epd.setRotation(1);
```

## 🎯 重构优化方案

### 方案1: 重构 [`main.cpp`](src/main.cpp)（优先级：高）

**目标**: 提高代码可读性和可维护性

**实施步骤**:

1. **提取初始化函数**:
```cpp
// 在 main.cpp 中添加辅助函数

void initializeSerial() {
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println("System starting up...");
}

void initializeManagers() {
    weatherManager.begin();
    timeManager.begin();
}

void initializeSensors() {
    if (sht40.begin()) {
        Serial.println("SHT40 initialized successfully");
    } else {
        Serial.println("Warning: SHT40 init failed");
    }
}

void initializeDisplay() {
    epd.begin();
    epd.setRotation(1);
    epd.setTimeFont(&DSEG7Modern_Bold28pt7b);
    epd.setWeatherSymbolFont(&Weather_Symbols_Regular9pt7b);
}

void initializeRTC() {
    if (rtc.begin()) {
        Serial.println("BM8563 RTC initialized successfully");
        rtc.resetInterrupts();  // 使用新添加的方法
    } else {
        Serial.println("Failed to initialize BM8563 RTC");
    }
}
```

2. **提取 WiFi 连接逻辑**:
```cpp
bool connectAndUpdateWiFi() {
    wifiManager.begin();
    bool connected = wifiManager.smartConnect();
    
    if (connected && !wifiManager.isConfigMode()) {
        Serial.println("WiFi connected successfully");
        timeManager.setWiFiConnected(true);
        
        if (weatherManager.shouldUpdateFromNetwork()) {
            Serial.println("Updating weather from network...");
            timeManager.updateNTPTime();
            weatherManager.updateWeather(true);
        }
        return true;
    } else if (wifiManager.isConfigMode()) {
        handleConfigMode();
        return false;  // 配网后会重启
    } else {
        Serial.println("WiFi failed, using cached data");
        timeManager.setWiFiConnected(false);
        return false;
    }
}

void handleConfigMode() {
    Serial.println("Entered config mode");
    
    String apName = wifiManager.getConfigPortalSSID();
    String ipAddress = wifiManager.getConfigPortalIP();
    epd.showConfigPortalInfo(apName, ipAddress);
    
    while (wifiManager.isConfigMode()) {
        wifiManager.handleConfigPortal();
        delay(100);
    }
    
    ESP.restart();
}
```

3. **提取数据采集和显示**:
```cpp
void updateAndDisplay() {
    // 获取天气信息
    WeatherInfo currentWeather = weatherManager.getCurrentWeather();
    DateTime currentTime = timeManager.getCurrentTime();
    
    // 读取传感器数据
    float temperature = NAN, humidity = NAN;
    if (!sht40.readTemperatureHumidity(temperature, humidity)) {
        Serial.println("Warning: Failed to read SHT40");
    }
    
    // 读取电池状态
    battery.begin();
    float batteryPercentage = battery.getBatteryPercentage();
    
    // 显示信息
    epd.showTimeDisplay(currentTime, currentWeather, 
                        temperature, humidity, batteryPercentage);
}
```

4. **重构后的 setup()**:
```cpp
void setup() {
    initializeSerial();
    initializeManagers();
    initializeSensors();
    initializeDisplay();
    initializeRTC();
    
    connectAndUpdateWiFi();
    updateAndDisplay();
    
    goToDeepSleep();
}
```

**效果**: 从 118 行减少到 10 行，逻辑清晰，易于维护。

### 方案2: 优化 BM8563 类（优先级：高）

**在 [`lib/BM8563/BM8563.h`](lib/BM8563/BM8563.h) 中添加**:
```cpp
/**
 * @brief 重置所有中断标志和禁用中断
 * @note 用于清除可能导致 INT 引脚拉低的状态
 */
void resetInterrupts() {
    clearTimerFlag();
    clearAlarmFlag();
    enableTimerInterrupt(false);
    enableAlarmInterrupt(false);
}

/**
 * @brief 配置深度睡眠唤醒定时器
 * @param seconds 睡眠时间（秒）
 * @note 自动清除中断标志并设置定时器
 */
void setupWakeupTimer(uint16_t seconds) {
    resetInterrupts();
    setTimer(seconds, BM8563_TIMER_1HZ);
    enableTimerInterrupt(true);
}
```

**在 [`lib/BM8563/BM8563.cpp`](lib/BM8563/BM8563.cpp) 中实现**:
```cpp
void BM8563::resetInterrupts() {
    clearTimerFlag();
    clearAlarmFlag();
    enableTimerInterrupt(false);
    enableAlarmInterrupt(false);
}

void BM8563::setupWakeupTimer(uint16_t seconds) {
    resetInterrupts();
    setTimer(seconds, BM8563_TIMER_1HZ);
    enableTimerInterrupt(true);
}
```

**使用**:
```cpp
// main.cpp
void initializeRTC() {
    if (rtc.begin()) {
        rtc.resetInterrupts();  // 简化的调用
    }
}

void goToDeepSleep() {
    Serial.println("Entering deep sleep...");
    rtc.setupWakeupTimer(DEEP_SLEEP_SECONDS);  // 一行搞定
    Serial.flush();
    delay(100);
    ESP.deepSleep(0);
}
```

### 方案3: 统一配置文件（优先级：中）

**更新 [`config.h.example`](config.h.example)**:
```cpp
#ifndef CONFIG_H
#define CONFIG_H

// ==================== 硬件配置 ====================

// I2C 引脚配置 (BM8563 RTC & SHT40 传感器)
#define I2C_SDA_PIN 2   // GPIO-2 (D4)
#define I2C_SCL_PIN 12  // GPIO-12 (D6)

// 墨水屏 SPI 引脚配置 (GDEY029T94)
#define EPD_CS_PIN   D8  // 片选
#define EPD_DC_PIN   D2  // 数据/命令
#define EPD_RST_PIN  D0  // 复位
#define EPD_BUSY_PIN D1  // 忙碌信号

// ==================== 系统配置 ====================

// 串口波特率 (ESP8266 ROM bootloader 默认波特率)
#define SERIAL_BAUD_RATE 74880

// 深度睡眠配置
#define DEEP_SLEEP_SECONDS 60  // 深度睡眠时间（秒），1分钟唤醒一次
#define RTC_TIMER_SECONDS  60  // RTC 定时器时间（必须与深度睡眠时间一致）

// 显示配置
#define DISPLAY_ROTATION 1  // 旋转角度：0=0°, 1=90°, 2=180°, 3=270°

// ==================== API 配置 ====================

// 高德地图 API 配置
// 申请地址: https://lbs.amap.com/
#define AMAP_API_KEY "your_amap_api_key_here"
#define CITY_CODE "110108"  // 城市代码，例如：110108为北京海淀区

// 天气更新间隔（秒）
#define WEATHER_UPDATE_INTERVAL 1800  // 30分钟

// ==================== WiFi 配置 ====================

// WiFi 默认配置（可选，如不配置则首次启动进入配网模式）
#define DEFAULT_WIFI_SSID "your_wifi_ssid"
#define DEFAULT_WIFI_PASSWORD "your_wifi_password"

// 自定义 MAC 地址（可选）
#define DEFAULT_MAC_ADDRESS "AA:BB:CC:DD:EE:FF"

// WiFi 连接超时（毫秒）
#define WIFI_CONNECT_TIMEOUT 30000  // 30秒

// ==================== 电池配置 ====================

// 电池电压范围（用于电量百分比计算）
#define BATTERY_MIN_VOLTAGE 3.0  // 最低电压（V）
#define BATTERY_MAX_VOLTAGE 4.2  // 最高电压（V）

#endif // CONFIG_H
```

**更新 [`main.cpp`](src/main.cpp) 使用新配置**:
```cpp
// 引脚定义改为使用 config.h 中的定义
BM8563 rtc(I2C_SDA_PIN, I2C_SCL_PIN);
GDEY029T94 epd(EPD_CS_PIN, EPD_DC_PIN, EPD_RST_PIN, EPD_BUSY_PIN);
SHT40 sht40(I2C_SDA_PIN, I2C_SCL_PIN);

void initializeSerial() {
    Serial.begin(SERIAL_BAUD_RATE);
}

void initializeDisplay() {
    epd.begin();
    epd.setRotation(DISPLAY_ROTATION);
    // ...
}

void goToDeepSleep() {
    rtc.setupWakeupTimer(DEEP_SLEEP_SECONDS);
    ESP.deepSleep(0);
}
```

### 方案4: 改进错误处理（优先级：中）

**在 [`main.cpp`](src/main.cpp) 顶部添加**:
```cpp
// 错误处理辅助函数
void logError(const char* component, const char* message) {
    Serial.print("[ERROR] ");
    Serial.print(component);
    Serial.print(": ");
    Serial.println(message);
}

void logWarning(const char* component, const char* message) {
    Serial.print("[WARN] ");
    Serial.print(component);
    Serial.print(": ");
    Serial.println(message);
}

void logInfo(const char* component, const char* message) {
    Serial.print("[INFO] ");
    Serial.print(component);
    Serial.print(": ");
    Serial.println(message);
}
```

**使用示例**:
```cpp
void initializeSensors() {
    if (sht40.begin()) {
        logInfo("SHT40", "Initialized successfully");
    } else {
        logWarning("SHT40", "Init failed, will use NAN values");
    }
}

void initializeRTC() {
    if (rtc.begin()) {
        logInfo("RTC", "Initialized successfully");
        rtc.resetInterrupts();
    } else {
        logError("RTC", "Init failed - system may not work properly");
        // RTC 是关键组件，考虑是否需要重启
    }
}
```

### 方案5: 减少 String 使用（优先级：低）

**仅在关键路径优化**:
```cpp
// 当前代码
String cityCode = CITY_CODE;
WeatherManager weatherManager(amapApiKey, cityCode, &rtc, 512);

// 优化后
const char* cityCode = CITY_CODE;
WeatherManager weatherManager(amapApiKey, cityCode, &rtc, 512);
```

**注意**: 非关键路径（如配网界面）可以继续使用 String，不必过度优化。

### 方案6: 添加关键注释（优先级：低）

**在关键代码处添加注释**:
```cpp
void setup() {
    // ESP8266 ROM bootloader 使用 74880 波特率，保持一致便于查看启动信息
    Serial.begin(SERIAL_BAUD_RATE);
    
    // 初始化 RTC 并清除中断标志，防止 INT 引脚持续拉低
    if (rtc.begin()) {
        rtc.resetInterrupts();
    }
    
    // 旋转 90 度以适应 128x296 分辨率的横向显示
    epd.setRotation(DISPLAY_ROTATION);
    
    // ...
}

void goToDeepSleep() {
    // 配置 RTC 定时器在指定时间后通过 INT 引脚唤醒 ESP8266
    rtc.setupWakeupTimer(DEEP_SLEEP_SECONDS);
    
    // 进入深度睡眠，参数 0 表示无限期睡眠直到外部唤醒
    // 实际唤醒由 RTC 定时器触发硬件复位实现
    ESP.deepSleep(0);
}
```

## 📋 实施优先级

### 第一阶段（必须做）
1. ✅ **重构 [`main.cpp`](src/main.cpp)** - 提取函数，提高可读性
2. ✅ **优化 BM8563 类** - 添加 `resetInterrupts()` 和 `setupWakeupTimer()`
3. ✅ **统一配置文件** - 集中管理所有配置项

### 第二阶段（建议做）
4. ✅ **改进错误处理** - 添加日志辅助函数
5. ✅ **添加关键注释** - 说明特殊处理的原因

### 第三阶段（可选）
6. ⚪ **减少 String 使用** - 仅在关键路径优化

## 🎯 预期效果

### 代码质量
- **可读性**: [`main.cpp`](src/main.cpp) 从 209 行优化到约 100 行，逻辑清晰
- **可维护性**: 配置集中管理，修改方便
- **可靠性**: 统一的错误处理，更容易定位问题

### 性能
- **内存**: 减少不必要的 String 使用，降低内存碎片风险
- **功耗**: 无影响（保持现有优秀的功耗表现）

### 开发效率
- **调试**: 统一的日志格式，更容易定位问题
- **配置**: 修改配置只需编辑一个文件
- **扩展**: 清晰的代码结构，便于添加新功能

## ⚠️ 注意事项

1. **保持功能不变**: 重构不改变任何功能，只优化代码结构
2. **充分测试**: 每次修改后都要测试深度睡眠和唤醒机制
3. **逐步进行**: 一次只改一个模块，避免引入过多变化
4. **保留备份**: 重构前备份当前可工作的代码

## 📝 总结

这个项目的核心功能和架构设计都很好，主要问题是代码组织和细节处理。通过以上重构方案，可以在**不增加新功能**的前提下：

1. **提高代码可读性** - 清晰的函数划分和命名
2. **改善可维护性** - 集中的配置管理和统一的错误处理
3. **保持现有优势** - 不影响功耗、性能和稳定性

重构工作量不大，预计 1-2 天即可完成，但会显著提升代码质量和后续维护效率。

---

**文档版本**: 2.0  
**更新日期**: 2026-01-12  
**重点**: 代码重构优化，不增加新功能
