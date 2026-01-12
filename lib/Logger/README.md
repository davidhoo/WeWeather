# Logger 库

统一的日志管理库，用于 WeWeather 项目中的所有日志输出。

## 功能特性

- 支持多个日志级别：DEBUG、INFO、WARNING、ERROR
- 使用 F() 宏将字符串存储在 Flash 中以节省 RAM
- 支持日志级别过滤
- 统一的日志格式：`[LEVEL] Component: Message`

## 使用方法

### 1. 初始化

在程序开始时调用 `Logger::begin()` 初始化日志系统：

```cpp
#include "../lib/Logger/Logger.h"

void setup() {
  // 使用默认波特率 74880 和 DEBUG 级别
  Logger::begin();
  
  // 或者自定义波特率和最小日志级别
  Logger::begin(115200, LOG_LEVEL_INFO);
}
```

### 2. 输出日志

使用不同的日志级别方法输出日志：

```cpp
// 调试信息
Logger::debug(F("Component"), F("Debug message"));

// 一般信息
Logger::info(F("Component"), F("Info message"));

// 警告信息
Logger::warning(F("Component"), F("Warning message"));

// 错误信息
Logger::error(F("Component"), F("Error message"));
```

### 3. 输出带数值的日志

为了避免使用 String 拼接占用 RAM，Logger 提供了专门的方法来输出带数值的日志：

```cpp
// 输出浮点数（可指定小数位数）
float temperature = 25.6;
Logger::infoValue(F("SHT40"), F("Temperature:"), temperature, F("°C"), 1);
// 输出：[INFO] SHT40: Temperature: 25.6 °C

// 输出整数
int rawADC = 512;
Logger::infoValue(F("Battery"), F("Raw ADC:"), rawADC);
// 输出：[INFO] Battery: Raw ADC: 512

// 不带后缀单位
Logger::infoValue(F("Component"), F("Value:"), 3.14159, nullptr, 3);
// 输出：[INFO] Component: Value: 3.142
```

**优势：**
- 所有字符串使用 F() 宏存储在 Flash 中
- 避免 String 对象的动态内存分配
- 减少 RAM 占用，提高系统稳定性

### 4. 日志级别

可以动态设置最小日志级别，低于此级别的日志将不会输出：

```cpp
// 只显示 WARNING 和 ERROR 级别的日志
Logger::setLogLevel(LOG_LEVEL_WARNING);
```

日志级别优先级（从低到高）：
- `LOG_LEVEL_DEBUG` - 调试信息
- `LOG_LEVEL_INFO` - 一般信息
- `LOG_LEVEL_WARNING` - 警告信息
- `LOG_LEVEL_ERROR` - 错误信息

### 5. 支持普通字符串

除了 Flash 字符串（F() 宏），Logger 也支持普通字符串：

```cpp
const char* component = "MyComponent";
const char* message = "Dynamic message";
Logger::info(component, message);
```

## 输出格式

日志输出格式为：`[LEVEL] Component: Message`

示例：
```
[INFO] System: Starting up...
[WARN] SHT40: Init failed, will use NAN values
[ERROR] RTC: Failed to initialize BM8563
```

## 迁移指南

### 从 Serial.print/println 迁移

**旧代码（简单消息）：**
```cpp
Serial.print("[INFO] Component: ");
Serial.println("Message");
```

**新代码：**
```cpp
Logger::info(F("Component"), F("Message"));
```

**旧代码（带数值）：**
```cpp
Serial.print(F("[INFO] SHT40: Temperature: "));
Serial.print(temperature);
Serial.println(F(" °C"));
```

**新代码：**
```cpp
Logger::infoValue(F("SHT40"), F("Temperature:"), temperature, F("°C"), 1);
```

**避免使用（占用 RAM）：**
```cpp
// ❌ 不推荐：使用 String 拼接
String msg = "Temperature: " + String(temperature) + " °C";
Logger::info("SHT40", msg.c_str());

// ✅ 推荐：使用 infoValue 方法
Logger::infoValue(F("SHT40"), F("Temperature:"), temperature, F("°C"), 1);
```

### 从 main.cpp 的旧日志函数迁移

**旧代码：**
```cpp
logInfo(F("Component"), F("Message"));
logWarning(F("Component"), F("Message"));
logError(F("Component"), F("Message"));
```

**新代码：**
```cpp
Logger::info(F("Component"), F("Message"));
Logger::warning(F("Component"), F("Message"));
Logger::error(F("Component"), F("Message"));
```

## 建议的组件名称

为了保持日志的一致性，建议使用以下组件名称：

- `System` - 系统级别的日志
- `RTC` - BM8563 RTC 模块
- `WiFi` - WiFi 连接相关
- `Weather` - 天气数据相关
- `TimeManager` - 时间管理
- `SHT40` - 温湿度传感器
- `Battery` - 电池监控
- `Display` - 显示相关
- `Power` - 电源管理

## 待迁移的库

以下库文件仍在使用 `Serial.print/println`，建议逐步迁移到 Logger：

1. **lib/TimeManager/TimeManager.cpp** - 19 处使用
2. **lib/WiFiManager/WiFiManager.cpp** - 58 处使用
3. **lib/WeatherManager/WeatherManager.cpp** - 46 处使用
4. **lib/GDEY029T94/GDEY029T94.cpp** - 16 处使用

迁移时需要：
1. 在头文件中包含 `#include "../Logger/Logger.h"`
2. 将所有 `Serial.print/println` 替换为相应的 Logger 方法
3. 根据日志内容选择合适的日志级别

## 注意事项

1. **内存优化**：始终使用 F() 宏包裹字符串字面量，将其存储在 Flash 而不是 RAM 中
2. **性能考虑**：在生产环境中可以设置较高的日志级别（如 INFO 或 WARNING）以减少串口输出
3. **初始化顺序**：确保在使用任何日志功能之前调用 `Logger::begin()`
