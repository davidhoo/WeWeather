#include "LogManager.h"
#include <stdarg.h>

// 静态成员变量初始化
LogLevel LogManager::_currentLevel = LOG_INFO;
bool LogManager::_timestampEnabled = false;
bool LogManager::_initialized = false;

// 全局实例
LogManager Logger;

void LogManager::begin(unsigned long baudRate, LogLevel level) {
    if (!_initialized) {
        Serial.begin(baudRate);
        _initialized = true;
    }
    _currentLevel = level;
    info(F("LogManager initialized"));
}

void LogManager::setLogLevel(LogLevel level) {
    _currentLevel = level;
    infof(F("Log level set to: %s"), _getLevelString(level));
}

LogLevel LogManager::getLogLevel() {
    return _currentLevel;
}

void LogManager::enableTimestamp(bool enable) {
    _timestampEnabled = enable;
    infof(F("Timestamp %s"), enable ? F("enabled") : F("disabled"));
}

// F() 宏版本的日志函数
void LogManager::error(const __FlashStringHelper* message) {
    _print(LOG_ERROR, message);
}

void LogManager::warn(const __FlashStringHelper* message) {
    _print(LOG_WARN, message);
}

void LogManager::info(const __FlashStringHelper* message) {
    _print(LOG_INFO, message);
}

void LogManager::debug(const __FlashStringHelper* message) {
    _print(LOG_DEBUG, message);
}

// String 版本的日志函数
void LogManager::error(const String& message) {
    _print(LOG_ERROR, message);
}

void LogManager::warn(const String& message) {
    _print(LOG_WARN, message);
}

void LogManager::info(const String& message) {
    _print(LOG_INFO, message);
}

void LogManager::debug(const String& message) {
    _print(LOG_DEBUG, message);
}

// C字符串版本的日志函数
void LogManager::error(const char* message) {
    _print(LOG_ERROR, message);
}

void LogManager::warn(const char* message) {
    _print(LOG_WARN, message);
}

void LogManager::info(const char* message) {
    _print(LOG_INFO, message);
}

void LogManager::debug(const char* message) {
    _print(LOG_DEBUG, message);
}

// 格式化日志输出函数
void LogManager::errorf(const __FlashStringHelper* format, ...) {
    va_list args;
    va_start(args, format);
    _printf(LOG_ERROR, format, args);
    va_end(args);
}

void LogManager::warnf(const __FlashStringHelper* format, ...) {
    va_list args;
    va_start(args, format);
    _printf(LOG_WARN, format, args);
    va_end(args);
}

void LogManager::infof(const __FlashStringHelper* format, ...) {
    va_list args;
    va_start(args, format);
    _printf(LOG_INFO, format, args);
    va_end(args);
}

void LogManager::debugf(const __FlashStringHelper* format, ...) {
    va_list args;
    va_start(args, format);
    _printf(LOG_DEBUG, format, args);
    va_end(args);
}

void LogManager::printSeparator(char character, int length) {
    if (_currentLevel >= LOG_INFO) {
        for (int i = 0; i < length; i++) {
            Serial.print(character);
        }
        Serial.println();
    }
}

void LogManager::printKeyValue(const __FlashStringHelper* key, const String& value) {
    if (_currentLevel >= LOG_INFO) {
        _printPrefix(LOG_INFO);
        Serial.print(key);
        Serial.print(F(": "));
        Serial.println(value);
    }
}

void LogManager::printKeyValue(const __FlashStringHelper* key, int value) {
    if (_currentLevel >= LOG_INFO) {
        _printPrefix(LOG_INFO);
        Serial.print(key);
        Serial.print(F(": "));
        Serial.println(value);
    }
}

void LogManager::printKeyValue(const __FlashStringHelper* key, float value, int decimals) {
    if (_currentLevel >= LOG_INFO) {
        _printPrefix(LOG_INFO);
        Serial.print(key);
        Serial.print(F(": "));
        Serial.println(value, decimals);
    }
}

void LogManager::printKeyValue(const __FlashStringHelper* key, bool value) {
    if (_currentLevel >= LOG_INFO) {
        _printPrefix(LOG_INFO);
        Serial.print(key);
        Serial.print(F(": "));
        Serial.println(value ? F("true") : F("false"));
    }
}

// 私有辅助函数实现
void LogManager::_printPrefix(LogLevel level) {
    if (!_initialized) return;
    
    if (_timestampEnabled) {
        _printTimestamp();
    }
    
    Serial.print(F("["));
    Serial.print(_getLevelString(level));
    Serial.print(F("] "));
}

void LogManager::_printTimestamp() {
    unsigned long currentTime = millis();
    unsigned long seconds = currentTime / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    
    Serial.print(F("["));
    if (hours < 10) Serial.print(F("0"));
    Serial.print(hours % 24);
    Serial.print(F(":"));
    if ((minutes % 60) < 10) Serial.print(F("0"));
    Serial.print(minutes % 60);
    Serial.print(F(":"));
    if ((seconds % 60) < 10) Serial.print(F("0"));
    Serial.print(seconds % 60);
    Serial.print(F("."));
    unsigned long ms = currentTime % 1000;
    if (ms < 100) Serial.print(F("0"));
    if (ms < 10) Serial.print(F("0"));
    Serial.print(ms);
    Serial.print(F("] "));
}

const char* LogManager::_getLevelString(LogLevel level) {
    switch (level) {
        case LOG_ERROR: return "ERROR";
        case LOG_WARN:  return "WARN ";
        case LOG_INFO:  return "INFO ";
        case LOG_DEBUG: return "DEBUG";
        default:        return "UNKN ";
    }
}

void LogManager::_print(LogLevel level, const __FlashStringHelper* message) {
    if (!_initialized || level > _currentLevel) return;
    
    _printPrefix(level);
    Serial.println(message);
}

void LogManager::_print(LogLevel level, const String& message) {
    if (!_initialized || level > _currentLevel) return;
    
    _printPrefix(level);
    Serial.println(message);
}

void LogManager::_print(LogLevel level, const char* message) {
    if (!_initialized || level > _currentLevel) return;
    
    _printPrefix(level);
    Serial.println(message);
}

void LogManager::_printf(LogLevel level, const __FlashStringHelper* format, va_list args) {
    if (!_initialized || level > _currentLevel) return;
    
    _printPrefix(level);
    
    // 创建缓冲区用于格式化字符串
    char buffer[256];
    vsnprintf_P(buffer, sizeof(buffer), (const char*)format, args);
    Serial.println(buffer);
}