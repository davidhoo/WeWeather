#include "WeatherManager.h"
#include "../LogManager/LogManager.h"
#include "../../config.h"

WeatherManager::WeatherManager(const char* apiKey, const String& cityCode, BM8563* rtc, int eepromSize)
  : _apiKey(apiKey), _cityCode(cityCode), _rtc(rtc), _updateIntervalSeconds(WEATHER_UPDATE_INTERVAL) {
  // 创建配置管理器实例，使用地址0与串口配置共享同一个EEPROM存储
  _configManager = new ConfigManager<ConfigData>(0, eepromSize);
  
  // 初始化默认天气信息
  initializeDefaultWeather();
}

WeatherManager::~WeatherManager() {
  if (_configManager) {
    delete _configManager;
    _configManager = nullptr;
  }
}

void WeatherManager::begin() {
  _configManager->begin();
  LOG_INFO("WeatherManager initialized");
  
  // 尝试从配置读取天气信息
  if (!readWeatherFromStorage()) {
    LOG_INFO("Using default weather values");
  }
}

WeatherInfo WeatherManager::getCurrentWeather() {
  return _currentWeather;
}

bool WeatherManager::updateWeather(bool forceUpdate) {
  if (forceUpdate || shouldUpdateFromNetwork()) {
    LOG_INFO("Updating weather from network...");
    if (fetchWeatherFromNetwork()) {
      writeWeatherToStorage();
      return true;
    } else {
      LOG_WARN("Failed to fetch weather from network, using cached data");
      return false;
    }
  } else {
    LOG_INFO("Using cached weather data");
    return true;
  }
}

bool WeatherManager::shouldUpdateFromNetwork() {
  unsigned long lastUpdateTime = getLastUpdateTime();
  
  // 如果从未更新过，应该更新
  if (lastUpdateTime == 0) {
    LOG_INFO("No previous weather data, need to update from network");
    return true;
  }
  
  unsigned long currentTime = getCurrentUnixTimestamp();
  
  // 如果无法获取当前时间，需要更新
  if (currentTime == 0 || currentTime == 1 || currentTime == (time_t)-1) {
    LOG_WARN("Cannot get current time, need to update from network");
    return true;
  }
  
  // 计算时间差（秒）
  unsigned long timeDiffSeconds = currentTime - lastUpdateTime;
  
  // 检查是否超过了更新间隔
  bool shouldUpdate = timeDiffSeconds >= _updateIntervalSeconds;
  
  LOG_INFO_F("Current Unix time: %lu, Last update: %lu, Diff: %lu seconds (%lu minutes), %s",
             currentTime, lastUpdateTime, timeDiffSeconds,
             timeDiffSeconds / 60, shouldUpdate ? "need to update from network" : "using cached data");
  
  return shouldUpdate;
}

bool WeatherManager::fetchWeatherFromNetwork() {
  HTTPClient http;
  WiFiClientSecure client;
  
  // API URL
  String url = "https://restapi.amap.com/v3/weather/weatherInfo?key=" + String(_apiKey) + "&city=" + _cityCode + "&extensions=base&output=JSON";
  
  LogManager::info(String(F("Fetching weather data from: ")) + url);
  
  client.setInsecure(); // 跳过SSL证书验证
  http.begin(client, url);
  http.setTimeout(5000); // 5秒超时
  
  int httpResponseCode = http.GET();
  
  if (httpResponseCode == 200) {
    String payload = http.getString();
    LogManager::info(String(F("Weather data received: ")) + payload);
    
    // 解析JSON数据
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
      LogManager::warn(String(F("Failed to parse JSON: ")) + String(error.c_str()));
      http.end();
      return false;
    }
    
    // 检查status是否为"1"
    String status = doc["status"].as<String>();
    if (status != "1") {
      LogManager::warn(String(F("API returned error status: ")) + status);
      http.end();
      return false;
    }
    
    // 获取lives数据
    JsonObject lives = doc["lives"][0];
    
    // 更新WeatherInfo
    _currentWeather.Temperature = lives["temperature"].as<float>();
    _currentWeather.Humidity = lives["humidity"].as<int>();
    _currentWeather.WindDirection = lives["winddirection"].as<String>();
    _currentWeather.WindSpeed = lives["windpower"].as<String>();
    _currentWeather.Weather = lives["weather"].as<String>();
    
    // 根据天气状况设置符号
    _currentWeather.Symbol = mapWeatherToSymbol(_currentWeather.Weather);
    
    LOG_INFO("Weather updated successfully");
    LogManager::info(String(F("Temperature: ")) + String(_currentWeather.Temperature));
    LogManager::info(String(F("Humidity: ")) + String(_currentWeather.Humidity));
    LogManager::info(String(F("Wind Direction: ")) + _currentWeather.WindDirection);
    LogManager::info(String(F("Wind Speed: ")) + _currentWeather.WindSpeed);
    LogManager::info(String(F("Weather: ")) + _currentWeather.Weather);
    LogManager::info(String(F("Symbol: ")) + String(_currentWeather.Symbol));
    
    http.end();
    return true;
  } else {
    LogManager::warn(String(F("HTTP request failed with code: ")) + String(httpResponseCode));
    http.end();
    return false;
  }
}

bool WeatherManager::readWeatherFromStorage() {
  ConfigData configData;
  
  // 使用ConfigManager读取数据
  if (!_configManager->read(configData)) {
    LOG_WARN("Failed to read weather config from storage, using default values");
    return false;
  }
  
  // 检查上次更新时间是否为0（首次使用）
  if (configData.lastUpdateTime == 0) {
    LOG_INFO("No weather config stored, using default values");
    return false;
  }
  
  // 转换数据格式
  convertFromConfigData(configData, _currentWeather);
  
  LOG_INFO("Weather config read from storage successfully");
  LogManager::info(String(F("Temperature: ")) + String(_currentWeather.Temperature));
  LogManager::info(String(F("Humidity: ")) + String(_currentWeather.Humidity));
  LogManager::info(String(F("Weather: ")) + _currentWeather.Weather);
  LogManager::info(String(F("Last Update: ")) + String(configData.lastUpdateTime));
  
  return true;
}

bool WeatherManager::writeWeatherToStorage() {
  ConfigData configData;
  
  // 转换数据格式
  convertToConfigData(_currentWeather, configData);
  
  // 获取当前时间戳
  unsigned long currentTime = getCurrentUnixTimestamp();
  if (currentTime != 0 && currentTime != 1 && currentTime != (time_t)-1) {
    configData.lastUpdateTime = currentTime;
  } else {
    LOG_ERROR("Failed to get current timestamp for weather update");
    return false;
  }
  
  // 使用ConfigManager写入数据
  bool success = _configManager->write(configData);
  
  if (success) {
    LOG_INFO("Weather config written to storage successfully");
    LogManager::info(String(F("Temperature: ")) + String(configData.temperature));
    LogManager::info(String(F("Humidity: ")) + String(configData.humidity));
    LogManager::info(String(F("Weather: ")) + String(configData.weather));
    LogManager::info(String(F("Last Update: ")) + String(configData.lastUpdateTime));
  } else {
    LOG_ERROR("Failed to write weather config to storage");
  }
  
  return success;
}

void WeatherManager::setUpdateInterval(unsigned long intervalSeconds) {
  _updateIntervalSeconds = intervalSeconds;
}

unsigned long WeatherManager::getLastUpdateTime() {
  ConfigData configData;
  
  // 使用ConfigManager读取数据
  if (!_configManager->read(configData)) {
    return 0;
  }
  
  return configData.lastUpdateTime;
}

bool WeatherManager::setUpdateTime(unsigned long timestamp) {
  ConfigData configData;
  
  // 读取现有的数据
  if (!_configManager->read(configData)) {
    LOG_ERROR("Weather config not found, cannot update timestamp");
    return false;
  }
  
  // 更新时间戳
  configData.lastUpdateTime = timestamp;
  
  // 使用ConfigManager写入数据
  bool success = _configManager->write(configData);
  
  if (success) {
    LOG_INFO("Timestamp updated successfully");
    LogManager::info(String(F("New timestamp: ")) + String(timestamp));
  } else {
    LOG_ERROR("Failed to update timestamp");
  }
  
  return success;
}

void WeatherManager::clearWeatherData() {
  // 使用ConfigManager清除数据
  _configManager->clear();
  LOG_INFO("Weather config cleared from storage");
}

char WeatherManager::mapWeatherToSymbol(const String& weather) {
  // Weather symbol mapping:
  // n=晴(sunny), d=雪(snow), m=雨(rain), l=雾(fog), c=阴(overcast), o=多云(cloudy), k=雷雨(thunderstorm)
  
  if (weather.indexOf("晴") >= 0) {
    return 'n'; // 晴天
  } else if (weather.indexOf("雷") >= 0 && weather.indexOf("雨") >= 0) {
    return 'k'; // 雷雨
  } else if (weather.indexOf("雪") >= 0) {
    return 'd'; // 雪
  } else if (weather.indexOf("雨") >= 0) {
    return 'm'; // 雨
  } else if (weather.indexOf("雷") >= 0) {
    return 'a'; // 雷
  } else if (weather.indexOf("雾") >= 0) {
    return 'l'; // 雾
  } else if (weather.indexOf("阴") >= 0) {
    return 'c'; // 阴
  } else if (weather.indexOf("多云") >= 0) {
    return 'o'; // 多云
  } else if (weather.indexOf("少云") >= 0) {
    return 'p'; // 少云
  } else if (weather.indexOf("晴间多云") >= 0) {
    return 'c'; // 晴间多云
  } else if (weather.indexOf("风") >= 0) {
    return 'f'; // 风
  } else if (weather.indexOf("冷") >= 0) {
    return 'e'; // 冷
  } else if (weather.indexOf("热") >= 0) {
    return 'h'; // 热
  } else {
    return 'n'; // 默认晴天
  }
}

void WeatherManager::initializeDefaultWeather() {
  _currentWeather.Temperature = 23.5;
  _currentWeather.Humidity = 65;
  _currentWeather.Symbol = 'n';
  _currentWeather.WindDirection = "北";
  _currentWeather.WindSpeed = "≤3";
  _currentWeather.Weather = "晴";
}

void WeatherManager::convertToConfigData(const WeatherInfo& weatherInfo, ConfigData& configData) {
  // 先尝试读取现有配置，保持非天气字段不变
  ConfigData existingConfig;
  if (_configManager->read(existingConfig)) {
    // 如果读取成功，复制现有的非天气配置
    strncpy(configData.amapApiKey, existingConfig.amapApiKey, sizeof(configData.amapApiKey) - 1);
    configData.amapApiKey[sizeof(configData.amapApiKey) - 1] = '\0';
    
    strncpy(configData.cityCode, existingConfig.cityCode, sizeof(configData.cityCode) - 1);
    configData.cityCode[sizeof(configData.cityCode) - 1] = '\0';
    
    strncpy(configData.wifiSSID, existingConfig.wifiSSID, sizeof(configData.wifiSSID) - 1);
    configData.wifiSSID[sizeof(configData.wifiSSID) - 1] = '\0';
    
    strncpy(configData.wifiPassword, existingConfig.wifiPassword, sizeof(configData.wifiPassword) - 1);
    configData.wifiPassword[sizeof(configData.wifiPassword) - 1] = '\0';
    
    strncpy(configData.macAddress, existingConfig.macAddress, sizeof(configData.macAddress) - 1);
    configData.macAddress[sizeof(configData.macAddress) - 1] = '\0';
  } else {
    // 如果读取失败，清空非天气字段
    memset(configData.amapApiKey, 0, sizeof(configData.amapApiKey));
    memset(configData.cityCode, 0, sizeof(configData.cityCode));
    memset(configData.wifiSSID, 0, sizeof(configData.wifiSSID));
    memset(configData.wifiPassword, 0, sizeof(configData.wifiPassword));
    memset(configData.macAddress, 0, sizeof(configData.macAddress));
  }
  
  // 只更新天气相关的数据
  configData.temperature = weatherInfo.Temperature;
  configData.humidity = weatherInfo.Humidity;
  configData.symbol = weatherInfo.Symbol;
  
  // 复制字符串，确保不超过缓冲区大小
  strncpy(configData.windDirection, weatherInfo.WindDirection.c_str(), sizeof(configData.windDirection) - 1);
  configData.windDirection[sizeof(configData.windDirection) - 1] = '\0';
  
  strncpy(configData.windSpeed, weatherInfo.WindSpeed.c_str(), sizeof(configData.windSpeed) - 1);
  configData.windSpeed[sizeof(configData.windSpeed) - 1] = '\0';
  
  strncpy(configData.weather, weatherInfo.Weather.c_str(), sizeof(configData.weather) - 1);
  configData.weather[sizeof(configData.weather) - 1] = '\0';
}

void WeatherManager::convertFromConfigData(const ConfigData& configData, WeatherInfo& weatherInfo) {
  weatherInfo.Temperature = configData.temperature;
  weatherInfo.Humidity = configData.humidity;
  weatherInfo.Symbol = configData.symbol;
  weatherInfo.WindDirection = String(configData.windDirection);
  weatherInfo.WindSpeed = String(configData.windSpeed);
  weatherInfo.Weather = String(configData.weather);
}

unsigned long WeatherManager::getCurrentUnixTimestamp() {
  // 尝试从系统获取当前Unix时间戳
  time_t now = time(nullptr);
  
  // 如果系统时间不可用，从RTC获取时间并转换为Unix时间戳
  if (now == 0 || now == 1) {
    LOG_WARN("System time not available, using RTC time");
    
    // 从RTC获取时间
    BM8563_Time rtcTime;
    if (!_rtc->getTime(&rtcTime)) {
      LOG_ERROR("Failed to read time from RTC");
      return 0;
    }
    
    // 将RTC时间转换为Unix时间戳
    struct tm timeinfo = {0};
    timeinfo.tm_year = 2000 + rtcTime.years - 1900;  // 转换为从1900开始的年份
    timeinfo.tm_mon = rtcTime.months - 1;               // 月份是0-11
    timeinfo.tm_mday = rtcTime.days;
    timeinfo.tm_hour = rtcTime.hours;
    timeinfo.tm_min = rtcTime.minutes;
    timeinfo.tm_sec = rtcTime.seconds;
    
    // 设置时区为UTC+8
    timeinfo.tm_isdst = -1;  // 让系统决定是否使用夏令时
    
    // 转换为Unix时间戳（UTC时间）
    now = mktime(&timeinfo);
    
    // 由于RTC时间是UTC+8时区，需要减去8小时的秒数，转换为UTC时间戳
    if (now != (time_t)-1) {
      now -= 8 * 3600;
    }
    
    if (now == (time_t)-1) {
      LOG_ERROR("Failed to convert RTC time to Unix timestamp");
      return 0;
    }
    
    LOG_DEBUG_F("RTC time converted to Unix timestamp: %ld", static_cast<long>(now));
  }
  
  return now;
}

String WeatherManager::translateWindDirection(const String& chineseDirection) {
  if (chineseDirection == "东") return "East";
  if (chineseDirection == "西") return "West";
  if (chineseDirection == "南") return "South";
  if (chineseDirection == "北") return "North";
  if (chineseDirection == "东北") return "Northeast";
  if (chineseDirection == "西北") return "Northwest";
  if (chineseDirection == "东南") return "Southeast";
  if (chineseDirection == "西南") return "Southwest";
  return chineseDirection; // 如果没有匹配的，返回原始值
}

String WeatherManager::formatWindSpeed(const String& windSpeed) {
  String formatted = windSpeed;
  formatted.replace("≤", "<=");
  formatted.replace("≥", ">=");
  return formatted;
}

String WeatherManager::getWeatherInfo(const WeatherInfo& currentWeather) {
  String weatherString = "";
  weatherString += String(currentWeather.Temperature, 0) + "C ";
  weatherString += String(currentWeather.Humidity) + "% ";
  weatherString += translateWindDirection(currentWeather.WindDirection) + " ";
  weatherString += formatWindSpeed(currentWeather.WindSpeed);
  return weatherString;
}

char WeatherManager::getWeatherSymbol(const WeatherInfo& currentWeather) {
  return currentWeather.Symbol;
}
