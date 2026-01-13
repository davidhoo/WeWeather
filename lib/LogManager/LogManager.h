#ifndef LOGMANAGER_H
#define LOGMANAGER_H

#include <Arduino.h>

// 日志级别枚举
enum LogLevel {
    LOG_NONE = 0,    // 不输出任何日志
    LOG_ERROR = 1,   // 错误信息
    LOG_WARN = 2,    // 警告信息
    LOG_INFO = 3,    // 一般信息
    LOG_DEBUG = 4    // 调试信息
};

class LogManager {
public:
    // 初始化日志管理器
    static void begin(unsigned long baudRate = 115200, LogLevel level = LOG_INFO);
    
    // 设置日志级别
    static void setLogLevel(LogLevel level);
    
    // 获取当前日志级别
    static LogLevel getLogLevel();
    
    // 启用/禁用时间戳
    static void enableTimestamp(bool enable);
    
    // 日志输出函数 - 使用 F() 宏的版本
    static void error(const __FlashStringHelper* message);
    static void warn(const __FlashStringHelper* message);
    static void info(const __FlashStringHelper* message);
    static void debug(const __FlashStringHelper* message);
    
    // 日志输出函数 - 字符串版本
    static void error(const String& message);
    static void warn(const String& message);
    static void info(const String& message);
    static void debug(const String& message);
    
    // 日志输出函数 - C字符串版本
    static void error(const char* message);
    static void warn(const char* message);
    static void info(const char* message);
    static void debug(const char* message);
    
    // 格式化日志输出函数
    static void errorf(const __FlashStringHelper* format, ...);
    static void warnf(const __FlashStringHelper* format, ...);
    static void infof(const __FlashStringHelper* format, ...);
    static void debugf(const __FlashStringHelper* format, ...);
    
    // UTF-8安全的格式化日志方法（用于处理可能包含中文的字符串）
    static void errorUTF8(const char* format, ...);
    static void warnUTF8(const char* format, ...);
    static void infoUTF8(const char* format, ...);
    static void debugUTF8(const char* format, ...);
    
    // 打印分隔线
    static void printSeparator(char character = '=', int length = 30);
    
    // 打印键值对
    static void printKeyValue(const __FlashStringHelper* key, const String& value);
    static void printKeyValue(const __FlashStringHelper* key, int value);
    static void printKeyValue(const __FlashStringHelper* key, float value, int decimals = 2);
    static void printKeyValue(const __FlashStringHelper* key, bool value);

private:
    static LogLevel _currentLevel;
    static bool _timestampEnabled;
    static bool _initialized;
    
    // 内部辅助函数
    static void _printPrefix(LogLevel level);
    static void _printTimestamp();
    static const char* _getLevelString(LogLevel level);
    static void _print(LogLevel level, const __FlashStringHelper* message);
    static void _print(LogLevel level, const String& message);
    static void _print(LogLevel level, const char* message);
    static void _printf(LogLevel level, const __FlashStringHelper* format, va_list args);
    static void _printfUTF8(LogLevel level, const char* format, va_list args);
};

// 便捷宏定义
#define LOG_ERROR(msg) LogManager::error(F(msg))
#define LOG_WARN(msg) LogManager::warn(F(msg))
#define LOG_INFO(msg) LogManager::info(F(msg))
#define LOG_DEBUG(msg) LogManager::debug(F(msg))

#define LOG_ERROR_F(format, ...) LogManager::errorf(F(format), ##__VA_ARGS__)
#define LOG_WARN_F(format, ...) LogManager::warnf(F(format), ##__VA_ARGS__)
#define LOG_INFO_F(format, ...) LogManager::infof(F(format), ##__VA_ARGS__)
#define LOG_DEBUG_F(format, ...) LogManager::debugf(F(format), ##__VA_ARGS__)

// UTF-8安全的日志宏定义（用于处理可能包含中文的字符串）
#define LOG_ERROR_UTF8(format, ...) LogManager::errorUTF8(format, ##__VA_ARGS__)
#define LOG_WARN_UTF8(format, ...) LogManager::warnUTF8(format, ##__VA_ARGS__)
#define LOG_INFO_UTF8(format, ...) LogManager::infoUTF8(format, ##__VA_ARGS__)
#define LOG_DEBUG_UTF8(format, ...) LogManager::debugUTF8(format, ##__VA_ARGS__)

// 全局日志管理器实例
extern LogManager Logger;

#endif // LOGMANAGER_H