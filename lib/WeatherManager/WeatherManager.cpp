#include "WeatherManager.h"
#include "../../config.h"

WeatherManager::WeatherManager(const char* apiKey, const String& cityCode, BM8563* rtc, int eepromSize)
  : _apiKey(apiKey), _cityCode(cityCode), _rtc(rtc), _eepromSize(eepromSize), _eepromAddress(0), _updateIntervalSeconds(WEATHER_UPDATE_INTERVAL) {
  // 初始化默认天气信息
  initializeDefaultWeather();
}

void WeatherManager::begin() {
  EEPROM.begin(_eepromSize);
  Logger::info(F("WeatherManager"), F("Initialized"));

  // 尝试从EEPROM读取天气数据
  if (!readWeatherFromStorage()) {
    Logger::info(F("WeatherManager"), F("Using default weather values"));
  }
}

WeatherInfo WeatherManager::getCurrentWeather() {
  return _currentWeather;
}

bool WeatherManager::updateWeather(bool forceUpdate) {
  if (forceUpdate || shouldUpdateFromNetwork()) {
    Logger::info(F("WeatherManager"), F("Updating weather from network..."));
    if (fetchWeatherFromNetwork()) {
      writeWeatherToStorage();
      return true;
    } else {
      Logger::warning(F("WeatherManager"), F("Failed to fetch weather from network, using cached data"));
      return false;
    }
  } else {
    Logger::info(F("WeatherManager"), F("Using cached weather data"));
    return true;
  }
}

bool WeatherManager::shouldUpdateFromNetwork() {
  unsigned long lastUpdateTime = getLastUpdateTime();
  
  // 如果从未更新过，应该更新
  if (lastUpdateTime == 0) {
    Logger::info(F("WeatherManager"), F("No previous weather data, need to update from network"));
    return true;
  }
  
  unsigned long currentTime = getCurrentUnixTimestamp();
  
  // 如果无法获取当前时间，需要更新
  if (currentTime == 0 || currentTime == 1 || currentTime == (time_t)-1) {
    Logger::warning(F("WeatherManager"), F("Cannot get current time, need to update from network"));
    return true;
  }
  
  // 计算时间差（秒）
  unsigned long timeDiffSeconds = currentTime - lastUpdateTime;
  
  // 检查是否超过了更新间隔
  bool shouldUpdate = (timeDiffSeconds >= _updateIntervalSeconds);

  char buffer[128];
  snprintf(buffer, sizeof(buffer),
           "Current: %lu, Last: %lu, Diff: %lu sec (%lu min), %s",
           (unsigned long)currentTime, lastUpdateTime, timeDiffSeconds, timeDiffSeconds / 60,
           shouldUpdate ? "need update" : "using cache");
  Logger::debug("WeatherManager", buffer);

  return shouldUpdate;
}

bool WeatherManager::fetchWeatherFromNetwork() {
  HTTPClient http;
  WiFiClientSecure client;
  
  // API URL
  String url = "https://restapi.amap.com/v3/weather/weatherInfo?key=" + String(_apiKey) + "&city=" + _cityCode + "&extensions=base&output=JSON";
  
  Logger::info("WeatherManager", ("Fetching weather data from: " + url).c_str());
  
  client.setInsecure(); // 跳过SSL证书验证
  http.begin(client, url);
  http.setTimeout(5000); // 5秒超时
  
  int httpResponseCode = http.GET();
  
  if (httpResponseCode == 200) {
    String payload = http.getString();
    Logger::debug("WeatherManager", ("Weather data received: " + payload).c_str());
    
    // 解析JSON数据
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
      String msg = "Failed to parse JSON: " + String(error.c_str());
      Logger::error("WeatherManager", msg.c_str());
      http.end();
      return false;
    }

    // 检查API返回状态
    String status = doc["status"];
    if (status != "1") {
      String msg = "API returned error status: " + status;
      Logger::error("WeatherManager", msg.c_str());
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

    Logger::info(F("WeatherManager"), F("Weather updated successfully"));
    Logger::infoValue(F("WeatherManager"), F("Temperature:"), _currentWeather.Temperature, F("°C"), 1);
    Logger::infoValue(F("WeatherManager"), F("Humidity:"), _currentWeather.Humidity, F("%"));
    Logger::info("WeatherManager", ("Wind Direction: " + _currentWeather.WindDirection).c_str());
    Logger::info("WeatherManager", ("Wind Speed: " + _currentWeather.WindSpeed).c_str());
    Logger::info("WeatherManager", ("Weather: " + _currentWeather.Weather).c_str());
    char symbolMsg[32];
    snprintf(symbolMsg, sizeof(symbolMsg), "Symbol: %c", _currentWeather.Symbol);
    Logger::info("WeatherManager", symbolMsg);

    http.end();
    return true;
  } else {
    Logger::error("WeatherManager", ("HTTP request failed with code: " + String(httpResponseCode)).c_str());
    http.end();
    return false;
  }
}

bool WeatherManager::readWeatherFromStorage() {
  WeatherStorageData storageData;
  
  // 从EEPROM读取数据
  EEPROM.get(_eepromAddress, storageData);
  
  // 计算校验和
  byte storedChecksum = EEPROM.read(_eepromAddress + sizeof(WeatherStorageData));
  byte calculatedChecksum = calculateChecksum(storageData);
  
  // 检查校验和
  if (storedChecksum != calculatedChecksum) {
    Logger::warning(F("WeatherManager"), F("Weather data checksum mismatch, using default values"));
    return false;
  }

  // 检查是否有有效数据
  if (storageData.lastUpdateTime == 0) {
    Logger::info(F("WeatherManager"), F("No weather data stored, using default values"));
    return false;
  }
  
  // 转换数据格式
  convertFromStorageData(storageData, _currentWeather);

  Logger::info(F("WeatherManager"), F("Weather data read from EEPROM successfully"));
  Logger::infoValue(F("WeatherManager"), F("Temperature:"), _currentWeather.Temperature, F("°C"), 1);
  Logger::infoValue(F("WeatherManager"), F("Humidity:"), _currentWeather.Humidity, F("%"));
  Logger::info("WeatherManager", ("Weather: " + _currentWeather.Weather).c_str());
  Logger::infoValue(F("WeatherManager"), F("Last Update:"), (int)storageData.lastUpdateTime);

  return true;
}

bool WeatherManager::writeWeatherToStorage() {
  WeatherStorageData storageData;
  
  // 转换数据格式
  convertToStorageData(_currentWeather, storageData);
  
  // 获取当前时间戳
  unsigned long currentTime = getCurrentUnixTimestamp();
  if (currentTime != 0 && currentTime != 1 && currentTime != (time_t)-1) {
    storageData.lastUpdateTime = currentTime;
  } else {
    Logger::error(F("WeatherManager"), F("Failed to get current timestamp for weather update"));
    return false;
  }
  
  // 写入数据到EEPROM
  EEPROM.put(_eepromAddress, storageData);
  
  // 计算并写入校验和
  byte checksum = calculateChecksum(storageData);
  EEPROM.write(_eepromAddress + sizeof(WeatherStorageData), checksum);
  
  // 提交更改
  bool success = EEPROM.commit();
  
  if (success) {
    Logger::info(F("WeatherManager"), F("Weather data written to EEPROM successfully"));
    Logger::infoValue(F("WeatherManager"), F("Temperature:"), storageData.temperature, F("°C"), 1);
    Logger::infoValue(F("WeatherManager"), F("Humidity:"), storageData.humidity, F("%"));
    Logger::info("WeatherManager", ("Weather: " + String(storageData.weather)).c_str());
    Logger::infoValue(F("WeatherManager"), F("Last Update:"), (int)storageData.lastUpdateTime);
  } else {
    Logger::error(F("WeatherManager"), F("Failed to write weather data to EEPROM"));
  }
  
  return success;
}

void WeatherManager::setUpdateInterval(unsigned long intervalSeconds) {
  _updateIntervalSeconds = intervalSeconds;
}

unsigned long WeatherManager::getLastUpdateTime() {
  WeatherStorageData storageData;
  
  // 从EEPROM读取数据
  EEPROM.get(_eepromAddress, storageData);
  
  // 计算校验和
  byte storedChecksum = EEPROM.read(_eepromAddress + sizeof(WeatherStorageData));
  byte calculatedChecksum = calculateChecksum(storageData);
  
  // 检查校验和
  if (storedChecksum != calculatedChecksum) {
    return 0;
  }
  
  return storageData.lastUpdateTime;
}

bool WeatherManager::setUpdateTime(unsigned long timestamp) {
  // 读取现有的数据
  WeatherStorageData storageData;
  EEPROM.get(_eepromAddress, storageData);
  
  // 计算校验和
  byte storedChecksum = EEPROM.read(_eepromAddress + sizeof(WeatherStorageData));
  byte calculatedChecksum = calculateChecksum(storageData);
  
  // 检查校验和
  if (storedChecksum != calculatedChecksum) {
    Logger::error(F("WeatherManager"), F("Weather data checksum mismatch, cannot update timestamp"));
    return false;
  }
  
  // 更新时间戳
  storageData.lastUpdateTime = timestamp;
  
  // 写入数据到EEPROM
  EEPROM.put(_eepromAddress, storageData);
  
  // 计算并写入校验和
  byte checksum = calculateChecksum(storageData);
  EEPROM.write(_eepromAddress + sizeof(WeatherStorageData), checksum);
  
  // 提交更改
  bool success = EEPROM.commit();
  
  if (success) {
    Logger::info(F("WeatherManager"), F("Timestamp updated successfully"));
    Logger::infoValue(F("WeatherManager"), F("New timestamp:"), (int)timestamp);
  } else {
    Logger::error(F("WeatherManager"), F("Failed to update timestamp"));
  }
  
  return success;
}

void WeatherManager::clearWeatherData() {
  WeatherStorageData storageData = {0};
  
  // 写入空数据
  EEPROM.put(_eepromAddress, storageData);
  
  // 写入校验和
  byte checksum = calculateChecksum(storageData);
  EEPROM.write(_eepromAddress + sizeof(WeatherStorageData), checksum);
  
  // 提交更改
  EEPROM.commit();

  Logger::info(F("WeatherManager"), F("Weather data cleared from EEPROM"));
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

void WeatherManager::convertToStorageData(const WeatherInfo& weatherInfo, WeatherStorageData& storageData) {
  storageData.temperature = weatherInfo.Temperature;
  storageData.humidity = weatherInfo.Humidity;
  storageData.symbol = weatherInfo.Symbol;
  
  // 复制字符串，确保不超过缓冲区大小
  strncpy(storageData.windDirection, weatherInfo.WindDirection.c_str(), sizeof(storageData.windDirection) - 1);
  storageData.windDirection[sizeof(storageData.windDirection) - 1] = '\0';
  
  strncpy(storageData.windSpeed, weatherInfo.WindSpeed.c_str(), sizeof(storageData.windSpeed) - 1);
  storageData.windSpeed[sizeof(storageData.windSpeed) - 1] = '\0';
  
  strncpy(storageData.weather, weatherInfo.Weather.c_str(), sizeof(storageData.weather) - 1);
  storageData.weather[sizeof(storageData.weather) - 1] = '\0';
}

void WeatherManager::convertFromStorageData(const WeatherStorageData& storageData, WeatherInfo& weatherInfo) {
  weatherInfo.Temperature = storageData.temperature;
  weatherInfo.Humidity = storageData.humidity;
  weatherInfo.Symbol = storageData.symbol;
  weatherInfo.WindDirection = String(storageData.windDirection);
  weatherInfo.WindSpeed = String(storageData.windSpeed);
  weatherInfo.Weather = String(storageData.weather);
}

byte WeatherManager::calculateChecksum(const WeatherStorageData& data) {
  byte checksum = 0;
  const byte* ptr = (const byte*)&data;
  
  for (size_t i = 0; i < sizeof(WeatherStorageData); i++) {
    checksum ^= ptr[i];
  }
  
  return checksum;
}

unsigned long WeatherManager::getCurrentUnixTimestamp() {
  // 尝试从系统获取当前Unix时间戳
  time_t now = time(nullptr);
  
  // 如果系统时间不可用，从RTC获取时间并转换为Unix时间戳
  // 如果系统时间不可用，从RTC获取时间并转换为Unix时间戳
  if (now == 0 || now == 1) {
    Logger::debug(F("WeatherManager"), F("System time not available, using RTC time"));

    BM8563_Time rtcTime;
    if (!_rtc->getTime(&rtcTime)) {
      Logger::error(F("WeatherManager"), F("Failed to read time from RTC"));
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
      Logger::error(F("WeatherManager"), F("Failed to convert RTC time to Unix timestamp"));
      return 0;
    }

    Logger::debug("WeatherManager", ("RTC time converted to Unix timestamp: " + String((unsigned long)now)).c_str());
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
  // 格式化天气信息用于显示
  String weatherString = "";
  
  // 按照新格式显示天气信息: 22C 46% NortheEast ≤3
  weatherString += String(currentWeather.Temperature, 0) + "C ";
  weatherString += String(currentWeather.Humidity) + "% ";
  weatherString += translateWindDirection(currentWeather.WindDirection) + " ";
  weatherString += formatWindSpeed(currentWeather.WindSpeed);
  
  return weatherString;
}

char WeatherManager::getWeatherSymbol(const WeatherInfo& currentWeather) {
  // 返回天气符号字符
  return currentWeather.Symbol;
}