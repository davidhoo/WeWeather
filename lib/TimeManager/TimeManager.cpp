#include "TimeManager.h"
#include "../LogManager/LogManager.h"

// NTP 服务器配置
const char* TimeManager::NTP_SERVERS[] = {
    "ntp.aliyun.com",
    "ntp1.aliyun.com", 
    "ntp2.aliyun.com"
};

TimeManager::TimeManager(BM8563* rtc) {
    _rtc = rtc;
    _wifiConnected = false;
    _timeValid = false;
    initializeCurrentTime();
}
bool TimeManager::begin() {
    if (_rtc == nullptr) {
        LOG_ERROR("TimeManager: RTC pointer is null");
        return false;
    }
    
    LOG_INFO("TimeManager: Initializing...");
    
    // 从 RTC 读取时间
    if (readTimeFromRTC()) {
        _timeValid = true;
        LOG_INFO("TimeManager: Time loaded from RTC");
        LOG_DEBUG_F("TimeManager: Current time: %04d/%02d/%02d %02d:%02d:%02d",
                    2000 + _currentTime.year, _currentTime.month, _currentTime.day,
                    _currentTime.hour, _currentTime.minute, _currentTime.second);
        return true;
    } else {
        LOG_ERROR("TimeManager: Failed to read time from RTC");
        return false;
    }
}

bool TimeManager::updateNTPTime() {
    if (!_wifiConnected) {
        LOG_WARN("TimeManager: WiFi not connected, skipping NTP update");
        return false;
    }
    
    LOG_INFO("TimeManager: Updating time from NTP server...");
    
    // 配置 NTP 服务器和时区 (UTC+8 北京时间)
    configTime(8 * 3600, 0, NTP_SERVERS[0], NTP_SERVERS[1], NTP_SERVERS[2]);
    
    // 等待时间同步
    int retryCount = 0;
    while (time(nullptr) < 1000000000 && retryCount < NTP_MAX_RETRIES) {
        delay(500);
        retryCount++;
        LogManager::debug(F("."));
    }
    LogManager::debug(F(""));
    
    if (retryCount >= NTP_MAX_RETRIES) {
        LOG_ERROR("TimeManager: Failed to get time from NTP server");
        return false;
    }
    
    // 获取时间
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    // 更新系统时间
    _currentTime.year = (timeinfo->tm_year + 1900) % 100;  // 转换为两位数年份
    _currentTime.month = timeinfo->tm_mon + 1;            // tm_mon 是从0开始的
    _currentTime.day = timeinfo->tm_mday;
    _currentTime.hour = timeinfo->tm_hour;
    _currentTime.minute = timeinfo->tm_min;
    _currentTime.second = timeinfo->tm_sec;
    
    _timeValid = true;
    
    // 同步时间到 BM8563 RTC
    if (writeTimeToRTC(_currentTime)) {
        LOG_DEBUG_F("TimeManager: NTP time updated: %04d/%02d/%02d %02d:%02d:%02d",
                    2000 + _currentTime.year, _currentTime.month, _currentTime.day,
                    _currentTime.hour, _currentTime.minute, _currentTime.second);
        return true;
    } else {
        LOG_ERROR("TimeManager: Failed to write NTP time to RTC");
        return false;
    }
}

bool TimeManager::readTimeFromRTC() {
    if (_rtc == nullptr) {
        LOG_ERROR("TimeManager: RTC pointer is null");
        return false;
    }
    
    BM8563_Time rtcTime;
    
    if (_rtc->getTime(&rtcTime)) {
        // 更新 currentTime
        // 更新 currentTime
        _currentTime.second = rtcTime.seconds;
        _currentTime.minute = rtcTime.minutes;
        _currentTime.hour = rtcTime.hours;
        _currentTime.day = rtcTime.days;
        _currentTime.month = rtcTime.months;
        _currentTime.year = rtcTime.years;
        
        _timeValid = true;
        LOG_DEBUG_F("TimeManager: Time read from RTC: %04d/%02d/%02d %02d:%02d:%02d",
                    2000 + _currentTime.year, _currentTime.month, _currentTime.day,
                    _currentTime.hour, _currentTime.minute, _currentTime.second);
        return true;
    } else {
        LOG_ERROR("TimeManager: Failed to read time from RTC");
        _timeValid = false;
        return false;
    }
}

bool TimeManager::writeTimeToRTC(const DateTime& dt) {
    if (_rtc == nullptr) {
        LOG_ERROR("TimeManager: RTC pointer is null");
        return false;
    }
    
    BM8563_Time rtcTime;
    
    // 转换 DateTime 到 BM8563_Time
    rtcTime.seconds = dt.second;
    rtcTime.minutes = dt.minute;
    rtcTime.hours = dt.hour;
    rtcTime.days = dt.day;
    rtcTime.weekdays = 0; // 不使用
    rtcTime.months = dt.month;
    rtcTime.years = dt.year;
    
    if (_rtc->setTime(&rtcTime)) {
        LOG_DEBUG_F("TimeManager: Time written to RTC: %04d/%02d/%02d %02d:%02d:%02d",
                    2000 + dt.year, dt.month, dt.day,
                    dt.hour, dt.minute, dt.second);
        return true;
    } else {
        LOG_ERROR("TimeManager: Failed to write time to RTC");
        return false;
    }
}
DateTime TimeManager::getCurrentTime() const {
    return _currentTime;
}

void TimeManager::setCurrentTime(const DateTime& dt) {
    _currentTime = dt;
    _timeValid = true;
}

void TimeManager::setWiFiConnected(bool connected) {
    _wifiConnected = connected;
    if (connected) {
        LOG_INFO("TimeManager: WiFi connected, NTP sync available");
    } else {
        LOG_INFO("TimeManager: WiFi disconnected, NTP sync unavailable");
    }
}

bool TimeManager::isTimeValid() const {
    return _timeValid;
}

String TimeManager::getFormattedTimeString() const {
    if (!_timeValid) {
        return F("Invalid Time");
    }
    
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "20%02d/%02d/%02d %02d:%02d:%02d",
             _currentTime.year, _currentTime.month, _currentTime.day,
             _currentTime.hour, _currentTime.minute, _currentTime.second);
    return String(buffer);
}

void TimeManager::initializeCurrentTime() {
    _currentTime.year = 0;
    _currentTime.month = 1;
    _currentTime.day = 1;
    _currentTime.hour = 0;
    _currentTime.minute = 0;
    _currentTime.second = 0;
    _timeValid = false;
}

void TimeManager::printTimeDebug(const char* prefix, const DateTime& dt) {
    LOG_DEBUG_F("%s: %04d/%02d/%02d %02d:%02d:%02d",
                prefix, 2000 + dt.year, dt.month, dt.day,
                dt.hour, dt.minute, dt.second);
}

String TimeManager::getFormattedTime(const DateTime& currentTime) {
    char timeString[6];
    sprintf(timeString, "%02d:%02d", currentTime.hour, currentTime.minute);
    
    return String(timeString);
}

String TimeManager::getFormattedDate(const DateTime& currentTime) {
    // 获取完整年份（2000 + 两位数年份）
    int fullYear = 2000 + currentTime.year;
    
    // 获取星期几
    String dayOfWeek = getDayOfWeek(fullYear, currentTime.month, currentTime.day);
    
    char dateString[50];
    sprintf(dateString, "%04d/%02d/%02d %s", fullYear, currentTime.month, currentTime.day, dayOfWeek.c_str());
    
    return String(dateString);
}

String TimeManager::getDayOfWeek(int year, int month, int day) {
    // 使用Zeller公式计算星期几
    if (month < 3) {
        month += 12;
        year--;
    }
    
    int k = year % 100;
    int j = year / 100;
    
    int h = (day + ((13 * (month + 1)) / 5) + k + (k / 4) + (j / 4) - 2 * j) % 7;
    
    // 转换为星期几字符串（0=周六，1=周日，2=周一...）
    String days[] = {"Saturday", "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday"};
    return days[h];
}