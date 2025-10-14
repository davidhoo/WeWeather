#include "TimeManager.h"

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
        Serial.println("TimeManager: RTC pointer is null");
        return false;
    }
    
    Serial.println("TimeManager: Initializing...");
    
    // 从 RTC 读取时间
    if (readTimeFromRTC()) {
        _timeValid = true;
        Serial.println("TimeManager: Time loaded from RTC");
        printTimeDebug("TimeManager: Current time", _currentTime);
        return true;
    } else {
        Serial.println("TimeManager: Failed to read time from RTC");
        return false;
    }
}

bool TimeManager::updateNTPTime() {
    if (!_wifiConnected) {
        Serial.println("TimeManager: WiFi not connected, skipping NTP update");
        return false;
    }
    
    Serial.println("TimeManager: Updating time from NTP server...");
    
    // 配置 NTP 服务器和时区 (UTC+8 北京时间)
    configTime(8 * 3600, 0, NTP_SERVERS[0], NTP_SERVERS[1], NTP_SERVERS[2]);
    
    // 等待时间同步
    int retryCount = 0;
    while (time(nullptr) < 1000000000 && retryCount < NTP_MAX_RETRIES) {
        delay(500);
        retryCount++;
        Serial.print(".");
    }
    Serial.println();
    
    if (retryCount >= NTP_MAX_RETRIES) {
        Serial.println("TimeManager: Failed to get time from NTP server");
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
        printTimeDebug("TimeManager: NTP time updated", _currentTime);
        return true;
    } else {
        Serial.println("TimeManager: Failed to write NTP time to RTC");
        return false;
    }
}

bool TimeManager::readTimeFromRTC() {
    if (_rtc == nullptr) {
        Serial.println("TimeManager: RTC pointer is null");
        return false;
    }
    
    BM8563_Time rtcTime;
    
    if (_rtc->getTime(&rtcTime)) {
        // 更新 currentTime
        _currentTime.second = rtcTime.seconds;
        _currentTime.minute = rtcTime.minutes;
        _currentTime.hour = rtcTime.hours;
        _currentTime.day = rtcTime.days;
        _currentTime.month = rtcTime.months;
        _currentTime.year = rtcTime.years;
        
        _timeValid = true;
        printTimeDebug("TimeManager: Time read from RTC", _currentTime);
        return true;
    } else {
        Serial.println("TimeManager: Failed to read time from RTC");
        _timeValid = false;
        return false;
    }
}

bool TimeManager::writeTimeToRTC(const DateTime& dt) {
    if (_rtc == nullptr) {
        Serial.println("TimeManager: RTC pointer is null");
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
        printTimeDebug("TimeManager: Time written to RTC", dt);
        return true;
    } else {
        Serial.println("TimeManager: Failed to write time to RTC");
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
        Serial.println("TimeManager: WiFi connected, NTP sync available");
    } else {
        Serial.println("TimeManager: WiFi disconnected, NTP sync unavailable");
    }
}

bool TimeManager::isTimeValid() const {
    return _timeValid;
}

String TimeManager::getFormattedTimeString() const {
    if (!_timeValid) {
        return "Invalid Time";
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
    Serial.print(prefix);
    Serial.print(": ");
    Serial.print(2000 + dt.year);
    Serial.print("/");
    Serial.print(dt.month);
    Serial.print("/");
    Serial.print(dt.day);
    Serial.print(" ");
    Serial.print(dt.hour);
    Serial.print(":");
    Serial.print(dt.minute);
    Serial.print(":");
    Serial.println(dt.second);
}