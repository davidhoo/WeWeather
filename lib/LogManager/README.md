# LogManager 库

统一的日志管理库，用于 WeWeather 项目中的所有日志输出。

## 功能特性

- 支持多种日志级别（ERROR, WARN, INFO, DEBUG）
- 使用 F() 宏减少内存占用
- 可选的时间戳显示
- 格式化输出支持
- 便捷的键值对输出
- 分隔线打印功能

## 使用方法

### 初始化

```cpp
#include "LogManager.h"

void setup() {
    // 初始化日志管理器，波特率 115200，日志级别 INFO
    LogManager::begin(115200, LOG_INFO);
    
    // 启用时间戳（可选）
    LogManager::enableTimestamp(true);
}
```

### 基本日志输出

```cpp
// 使用便捷宏（推荐，自动使用 F() 宏）
LOG_ERROR("这是错误信息");
LOG_WARN("这是警告信息");
LOG_INFO("这是一般信息");
LOG_DEBUG("这是调试信息");

// 直接调用函数
LogManager::error(F("错误信息"));
LogManager::warn(F("警告信息"));
LogManager::info(F("一般信息"));
LogManager::debug(F("调试信息"));

// 动态字符串
String message = "动态消息";
LogManager::info(message);
```

### 格式化输出

```cpp
// 使用格式化宏
LOG_INFO_F("温度: %.1f°C, 湿度: %.1f%%", temperature, humidity);
LOG_ERROR_F("连接失败，错误代码: %d", errorCode);

// 直接调用格式化函数
LogManager::infof(F("电池电压: %.2fV"), batteryVoltage);
```

### 键值对输出

```cpp
LogManager::printKeyValue(F("SSID"), wifiSSID);
LogManager::printKeyValue(F("温度"), temperature, 1);  // 1位小数
LogManager::printKeyValue(F("连接状态"), isConnected);
```

### 分隔线

```cpp
LogManager::printSeparator();           // 默认 30个 '=' 字符
LogManager::printSeparator('-', 20);    // 20个 '-' 字符
```

## 日志级别

- `LOG_NONE`: 不输出任何日志
- `LOG_ERROR`: 只输出错误信息
- `LOG_WARN`: 输出警告和错误信息
- `LOG_INFO`: 输出一般、警告和错误信息（默认）
- `LOG_DEBUG`: 输出所有级别的信息

## 内存优化

- 所有固定字符串都使用 F() 宏存储在 Flash 内存中
- 减少 RAM 占用
- 提高程序稳定性

## 示例输出

```
[INFO ] LogManager initialized
[INFO ] 温度: 25.3°C
[WARN ] WiFi 连接不稳定
[ERROR] 传感器读取失败
[DEBUG] 调试信息: 变量值 = 123
```

启用时间戳后：

```
[00:01:23.456] [INFO ] LogManager initialized
[00:01:24.123] [INFO ] 温度: 25.3°C