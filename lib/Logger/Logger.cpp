/**
 * @file Logger.cpp
 * @brief 统一日志管理库实现
 * @version 1.0
 * @date 2026-01-12
 */

#include "Logger.h"

// 静态成员变量初始化
LogLevel Logger::minLogLevel = LOG_LEVEL_DEBUG;
bool Logger::initialized = false;

/**
 * @brief 初始化日志系统
 */
void Logger::begin(unsigned long baudRate, LogLevel minLevel) {
  if (!initialized) {
    Serial.begin(baudRate);
    minLogLevel = minLevel;
    initialized = true;
  }
}

/**
 * @brief 设置最小日志级别
 */
void Logger::setLogLevel(LogLevel level) {
  minLogLevel = level;
}

/**
 * @brief 获取日志级别标签
 */
const __FlashStringHelper* Logger::getLevelTag(LogLevel level) {
  switch (level) {
    case LOG_LEVEL_DEBUG:
      return F("[DEBUG]");
    case LOG_LEVEL_INFO:
      return F("[INFO]");
    case LOG_LEVEL_WARNING:
      return F("[WARN]");
    case LOG_LEVEL_ERROR:
      return F("[ERROR]");
    default:
      return F("[UNKNOWN]");
  }
}

/**
 * @brief 内部日志输出函数（Flash字符串版本）
 */
void Logger::log(LogLevel level, const __FlashStringHelper* component, const __FlashStringHelper* message) {
  // 检查日志级别是否满足输出条件
  if (level < minLogLevel) {
    return;
  }
  
  // 输出日志格式：[LEVEL] Component: Message
  Serial.print(getLevelTag(level));
  Serial.print(F(" "));
  Serial.print(component);
  Serial.print(F(": "));
  Serial.println(message);
}

/**
 * @brief 内部日志输出函数（普通字符串版本）
 */
void Logger::log(LogLevel level, const char* component, const char* message) {
  // 检查日志级别是否满足输出条件
  if (level < minLogLevel) {
    return;
  }
  
  // 输出日志格式：[LEVEL] Component: Message
  Serial.print(getLevelTag(level));
  Serial.print(F(" "));
  Serial.print(component);
  Serial.print(F(": "));
  Serial.println(message);
}

/**
 * @brief 输出调试级别日志（Flash字符串）
 */
void Logger::debug(const __FlashStringHelper* component, const __FlashStringHelper* message) {
  log(LOG_LEVEL_DEBUG, component, message);
}

/**
 * @brief 输出信息级别日志（Flash字符串）
 */
void Logger::info(const __FlashStringHelper* component, const __FlashStringHelper* message) {
  log(LOG_LEVEL_INFO, component, message);
}

/**
 * @brief 输出警告级别日志（Flash字符串）
 */
void Logger::warning(const __FlashStringHelper* component, const __FlashStringHelper* message) {
  log(LOG_LEVEL_WARNING, component, message);
}

/**
 * @brief 输出错误级别日志（Flash字符串）
 */
void Logger::error(const __FlashStringHelper* component, const __FlashStringHelper* message) {
  log(LOG_LEVEL_ERROR, component, message);
}

/**
 * @brief 输出调试级别日志（普通字符串）
 */
void Logger::debug(const char* component, const char* message) {
  log(LOG_LEVEL_DEBUG, component, message);
}

/**
 * @brief 输出信息级别日志（普通字符串）
 */
void Logger::info(const char* component, const char* message) {
  log(LOG_LEVEL_INFO, component, message);
}

/**
 * @brief 输出警告级别日志（普通字符串）
 */
void Logger::warning(const char* component, const char* message) {
  log(LOG_LEVEL_WARNING, component, message);
}

/**
 * @brief 输出错误级别日志（普通字符串）
 */
void Logger::error(const char* component, const char* message) {
  log(LOG_LEVEL_ERROR, component, message);
}

/**
 * @brief 内部日志输出函数（Flash组件名 + 普通消息）
 */
void Logger::log(LogLevel level, const __FlashStringHelper* component, const char* message) {
  // 检查日志级别是否满足输出条件
  if (level < minLogLevel) {
    return;
  }
  
  // 输出日志格式：[LEVEL] Component: Message
  Serial.print(getLevelTag(level));
  Serial.print(F(" "));
  Serial.print(component);
  Serial.print(F(": "));
  Serial.println(message);
}

/**
 * @brief 输出调试级别日志（Flash组件名 + 普通消息）
 */
void Logger::debug(const __FlashStringHelper* component, const char* message) {
  log(LOG_LEVEL_DEBUG, component, message);
}

/**
 * @brief 输出信息级别日志（Flash组件名 + 普通消息）
 */
void Logger::info(const __FlashStringHelper* component, const char* message) {
  log(LOG_LEVEL_INFO, component, message);
}

/**
 * @brief 输出警告级别日志（Flash组件名 + 普通消息）
 */
void Logger::warning(const __FlashStringHelper* component, const char* message) {
  log(LOG_LEVEL_WARNING, component, message);
}

/**
 * @brief 输出错误级别日志（Flash组件名 + 普通消息）
 */
void Logger::error(const __FlashStringHelper* component, const char* message) {
  log(LOG_LEVEL_ERROR, component, message);
}

/**
 * @brief 输出带浮点数值的信息日志
 */
void Logger::infoValue(const __FlashStringHelper* component, const __FlashStringHelper* prefix,
                       float value, const __FlashStringHelper* suffix, int decimals) {
  // 检查日志级别
  if (LOG_LEVEL_INFO < minLogLevel) {
    return;
  }
  
  // 输出格式：[INFO] Component: Prefix Value Suffix
  Serial.print(getLevelTag(LOG_LEVEL_INFO));
  Serial.print(F(" "));
  Serial.print(component);
  Serial.print(F(": "));
  Serial.print(prefix);
  Serial.print(value, decimals);
  if (suffix != nullptr) {
    Serial.print(F(" "));
    Serial.print(suffix);
  }
  Serial.println();
}

/**
 * @brief 输出带整数值的信息日志
 */
void Logger::infoValue(const __FlashStringHelper* component, const __FlashStringHelper* prefix,
                       int value, const __FlashStringHelper* suffix) {
  // 检查日志级别
  if (LOG_LEVEL_INFO < minLogLevel) {
    return;
  }
  
  // 输出格式：[INFO] Component: Prefix Value Suffix
  Serial.print(getLevelTag(LOG_LEVEL_INFO));
  Serial.print(F(" "));
  Serial.print(component);
  Serial.print(F(": "));
  Serial.print(prefix);
  Serial.print(value);
  if (suffix != nullptr) {
    Serial.print(F(" "));
    Serial.print(suffix);
  }
  Serial.println();
}
