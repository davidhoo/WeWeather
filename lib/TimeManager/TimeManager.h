#ifndef TIMEMANAGER_H
#define TIMEMANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <time.h>
#include "../BM8563/BM8563.h"
// 日期时间结构体
struct DateTime {
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;
};

class TimeManager {
public:
    // 构造函数
    TimeManager(BM8563* rtc);
    
    // 初始化时间管理器
    bool begin();
    
    // NTP 时间同步功能
    bool updateNTPTime();
    
    // BM8563 RTC 时间读写功能
    bool readTimeFromRTC();
    bool writeTimeToRTC(const DateTime& dt);
    
    // 获取当前时间
    DateTime getCurrentTime() const;
    
    // 设置当前时间
    void setCurrentTime(const DateTime& dt);
    
    // 检查 WiFi 连接状态（用于 NTP 同步）
    void setWiFiConnected(bool connected);
    
    // 时间有效性检查
    bool isTimeValid() const;
    
    // 格式化时间输出（调试用）
    String getFormattedTimeString() const;
    
private:
    BM8563* _rtc;
    DateTime _currentTime;
    bool _wifiConnected;
    bool _timeValid;
    
    // NTP 服务器配置
    static const char* NTP_SERVERS[];
    static const int NTP_TIMEOUT_SECONDS = 10;
    static const int NTP_MAX_RETRIES = 10;
    
    // 内部辅助函数
    void initializeCurrentTime();
    bool syncTimeFromNTP();
    void printTimeDebug(const char* prefix, const DateTime& dt);
};

#endif // TIMEMANAGER_H