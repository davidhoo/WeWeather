/**
 * @file Logger.h
 * @brief 统一日志管理库
 * @details 提供统一的日志输出接口，支持不同日志级别
 *          使用 F() 宏将字符串存储在 Flash 中以节省 RAM
 * @version 1.0
 * @date 2026-01-12
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

/**
 * @brief 日志级别枚举
 */
enum LogLevel {
  LOG_LEVEL_DEBUG,   // 调试信息
  LOG_LEVEL_INFO,    // 一般信息
  LOG_LEVEL_WARNING, // 警告信息
  LOG_LEVEL_ERROR    // 错误信息
};

/**
 * @class Logger
 * @brief 日志管理类
 * @details 提供统一的日志输出接口，支持设置日志级别过滤
 */
class Logger {
public:
  /**
   * @brief 初始化日志系统
   * @param baudRate 串口波特率，默认74880
   * @param minLevel 最小日志级别，默认DEBUG（显示所有日志）
   */
  static void begin(unsigned long baudRate = 74880, LogLevel minLevel = LOG_LEVEL_DEBUG);
  
  /**
   * @brief 设置最小日志级别
   * @param level 最小日志级别，低于此级别的日志将不会输出
   */
  static void setLogLevel(LogLevel level);
  
  /**
   * @brief 输出调试级别日志
   * @param component 组件名称（使用 F() 宏）
   * @param message 日志消息（使用 F() 宏）
   */
  static void debug(const __FlashStringHelper* component, const __FlashStringHelper* message);
  
  /**
   * @brief 输出信息级别日志
   * @param component 组件名称（使用 F() 宏）
   * @param message 日志消息（使用 F() 宏）
   */
  static void info(const __FlashStringHelper* component, const __FlashStringHelper* message);
  
  /**
   * @brief 输出警告级别日志
   * @param component 组件名称（使用 F() 宏）
   * @param message 日志消息（使用 F() 宏）
   */
  static void warning(const __FlashStringHelper* component, const __FlashStringHelper* message);
  
  /**
   * @brief 输出错误级别日志
   * @param component 组件名称（使用 F() 宏）
   * @param message 日志消息（使用 F() 宏）
   */
  static void error(const __FlashStringHelper* component, const __FlashStringHelper* message);
  
  /**
   * @brief 输出调试级别日志（支持普通字符串）
   * @param component 组件名称
   * @param message 日志消息
   */
  static void debug(const char* component, const char* message);
  
  /**
   * @brief 输出信息级别日志（支持普通字符串）
   * @param component 组件名称
   * @param message 日志消息
   */
  static void info(const char* component, const char* message);
  
  /**
   * @brief 输出警告级别日志（支持普通字符串）
   * @param component 组件名称
   * @param message 日志消息
   */
  static void warning(const char* component, const char* message);
  
  /**
   * @brief 输出错误级别日志（支持普通字符串）
   * @param component 组件名称
   * @param message 日志消息
   */
  static void error(const char* component, const char* message);
  
  /**
   * @brief 输出带数值的信息日志（优化版本，避免 String 拼接）
   * @param component 组件名称（使用 F() 宏）
   * @param prefix 前缀消息（使用 F() 宏）
   * @param value 数值
   * @param suffix 后缀消息（使用 F() 宏），可选
   * @param decimals 小数位数，默认2位
   */
  static void infoValue(const __FlashStringHelper* component, const __FlashStringHelper* prefix,
                        float value, const __FlashStringHelper* suffix = nullptr, int decimals = 2);
  
  /**
   * @brief 输出带整数值的信息日志
   * @param component 组件名称（使用 F() 宏）
   * @param prefix 前缀消息（使用 F() 宏）
   * @param value 整数值
   * @param suffix 后缀消息（使用 F() 宏），可选
   */
  static void infoValue(const __FlashStringHelper* component, const __FlashStringHelper* prefix,
                        int value, const __FlashStringHelper* suffix = nullptr);

private:
  static LogLevel minLogLevel;  // 最小日志级别
  static bool initialized;       // 是否已初始化
  
  /**
   * @brief 内部日志输出函数
   * @param level 日志级别
   * @param component 组件名称
   * @param message 日志消息
   */
  static void log(LogLevel level, const __FlashStringHelper* component, const __FlashStringHelper* message);
  
  /**
   * @brief 内部日志输出函数（支持普通字符串）
   * @param level 日志级别
   * @param component 组件名称
   * @param message 日志消息
   */
  static void log(LogLevel level, const char* component, const char* message);
  
  /**
   * @brief 获取日志级别标签
   * @param level 日志级别
   * @return 日志级别标签字符串
   */
  static const __FlashStringHelper* getLevelTag(LogLevel level);
};

#endif // LOGGER_H
