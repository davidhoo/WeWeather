#include "WeatherStorage.h"
#include "../GDEY029T94/GDEY029T94.h"

WeatherStorage::WeatherStorage(int eepromSize) : _eepromSize(eepromSize), _eepromAddress(0) {
}

void WeatherStorage::begin() {
  EEPROM.begin(_eepromSize);
  Serial.println("WeatherStorage EEPROM initialized");
}

bool WeatherStorage::readWeatherInfo(WeatherInfo& weatherInfo) {
  WeatherStorageData storageData;
  
  // 从EEPROM读取数据
  EEPROM.get(_eepromAddress, storageData);
  
  // 计算校验和
  byte storedChecksum = EEPROM.read(_eepromAddress + sizeof(WeatherStorageData));
  byte calculatedChecksum = calculateChecksum(storageData);
  
  // 检查校验和
  if (storedChecksum != calculatedChecksum) {
    Serial.println("Weather data checksum mismatch, using default values");
    // 使用默认值
    weatherInfo.Temperature = 23.5;
    weatherInfo.Humidity = 65;
    weatherInfo.Symbol = 'n';
    weatherInfo.WindDirection = "北";
    weatherInfo.WindSpeed = "≤3";
    weatherInfo.Weather = "晴";
    return false;
  }
  
  // 检查上次更新时间是否为0（首次使用）
  if (storageData.lastUpdateTime == 0) {
    Serial.println("No weather data stored, using default values");
    // 使用默认值
    weatherInfo.Temperature = 23.5;
    weatherInfo.Humidity = 65;
    weatherInfo.Symbol = 'n';
    weatherInfo.WindDirection = "北";
    weatherInfo.WindSpeed = "≤3";
    weatherInfo.Weather = "晴";
    return false;
  }
  
  // 转换数据格式
  convertFromStorageData(storageData, weatherInfo);
  
  Serial.println("Weather data read from EEPROM successfully");
  Serial.print("Temperature: ");
  Serial.println(weatherInfo.Temperature);
  Serial.print("Humidity: ");
  Serial.println(weatherInfo.Humidity);
  Serial.print("Weather: ");
  Serial.println(weatherInfo.Weather);
  Serial.print("Last Update: ");
  Serial.println(storageData.lastUpdateTime);
  
  return true;
}

bool WeatherStorage::writeWeatherInfo(const WeatherInfo& weatherInfo) {
  WeatherStorageData storageData;
  
  // 转换数据格式
  convertToStorageData(weatherInfo, storageData);
  
  // 设置当前时间为更新时间
  storageData.lastUpdateTime = millis();
  
  // 写入数据到EEPROM
  EEPROM.put(_eepromAddress, storageData);
  
  // 计算并写入校验和
  byte checksum = calculateChecksum(storageData);
  EEPROM.write(_eepromAddress + sizeof(WeatherStorageData), checksum);
  
  // 提交更改
  bool success = EEPROM.commit();
  
  if (success) {
    Serial.println("Weather data written to EEPROM successfully");
    Serial.print("Temperature: ");
    Serial.println(storageData.temperature);
    Serial.print("Humidity: ");
    Serial.println(storageData.humidity);
    Serial.print("Weather: ");
    Serial.println(storageData.weather);
    Serial.print("Last Update: ");
    Serial.println(storageData.lastUpdateTime);
  } else {
    Serial.println("Failed to write weather data to EEPROM");
  }
  
  return success;
}

bool WeatherStorage::shouldUpdateWeather(unsigned long intervalMs) {
  unsigned long lastUpdateTime = getLastUpdateTime();
  
  // 如果从未更新过，应该更新
  if (lastUpdateTime == 0) {
    return true;
  }
  
  // 检查是否超过了更新间隔
  unsigned long currentTime = millis();
  // 处理millis()溢出的情况
  if (currentTime < lastUpdateTime) {
    return true;
  }
  
  return (currentTime - lastUpdateTime) >= intervalMs;
}

unsigned long WeatherStorage::getLastUpdateTime() {
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

void WeatherStorage::clearWeatherData() {
  WeatherStorageData storageData = {0};
  
  // 写入空数据
  EEPROM.put(_eepromAddress, storageData);
  
  // 写入校验和
  byte checksum = calculateChecksum(storageData);
  EEPROM.write(_eepromAddress + sizeof(WeatherStorageData), checksum);
  
  // 提交更改
  EEPROM.commit();
  
  Serial.println("Weather data cleared from EEPROM");
}

void WeatherStorage::convertToStorageData(const WeatherInfo& weatherInfo, WeatherStorageData& storageData) {
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

void WeatherStorage::convertFromStorageData(const WeatherStorageData& storageData, WeatherInfo& weatherInfo) {
  weatherInfo.Temperature = storageData.temperature;
  weatherInfo.Humidity = storageData.humidity;
  weatherInfo.Symbol = storageData.symbol;
  weatherInfo.WindDirection = String(storageData.windDirection);
  weatherInfo.WindSpeed = String(storageData.windSpeed);
  weatherInfo.Weather = String(storageData.weather);
}

byte WeatherStorage::calculateChecksum(const WeatherStorageData& data) {
  byte checksum = 0;
  const byte* ptr = (const byte*)&data;
  
  for (size_t i = 0; i < sizeof(WeatherStorageData); i++) {
    checksum ^= ptr[i];
  }
  
  return checksum;
}