#include "UnifiedConfigManager.h"
#include "../LogManager/LogManager.h"
#include <functional>

UnifiedConfigManager::UnifiedConfigManager(int eepromSize) 
    : _configManager(nullptr), _initialized(false) {
    _configManager = new ConfigManager<ConfigData>(0, eepromSize);
}

UnifiedConfigManager::~UnifiedConfigManager() {
    if (_configManager) {
        delete _configManager;
        _configManager = nullptr;
    }
}

void UnifiedConfigManager::begin() {
    if (!_initialized) {
        _configManager->begin();
        _initialized = true;
        LOG_INFO("UnifiedConfigManager initialized");
        printCurrentConfig();
    }
}

String UnifiedConfigManager::getWiFiSSID() {
    return _getConfigValue(nullptr, DEFAULT_WIFI_SSID, offsetof(ConfigData, wifiSSID));
}

String UnifiedConfigManager::getWiFipassword() {
    return _getConfigValue(nullptr, DEFAULT_WIFI_PASSWORD, offsetof(ConfigData, wifiPassword));
}

String UnifiedConfigManager::getMacAddress() {
    return _getConfigValue(nullptr, DEFAULT_MAC_ADDRESS, offsetof(ConfigData, macAddress));
}

String UnifiedConfigManager::getAmapApiKey() {
    return _getConfigValue(nullptr, DEFAULT_AMAP_API_KEY, offsetof(ConfigData, amapApiKey));
}

String UnifiedConfigManager::getCityCode() {
    return _getConfigValue(nullptr, DEFAULT_CITY_CODE, offsetof(ConfigData, cityCode));
}

bool UnifiedConfigManager::setWiFiConfig(const String& ssid, const String& password) {
    return _updateConfig([&](ConfigData& config) {
        _safeStringCopy(config.wifiSSID, ssid.c_str(), sizeof(config.wifiSSID));
        _safeStringCopy(config.wifiPassword, password.c_str(), sizeof(config.wifiPassword));
    });
}

bool UnifiedConfigManager::setMacAddress(const String& macAddress) {
    return _updateConfig([&](ConfigData& config) {
        _safeStringCopy(config.macAddress, macAddress.c_str(), sizeof(config.macAddress));
    });
}

bool UnifiedConfigManager::setApiConfig(const String& apiKey, const String& cityCode) {
    return _updateConfig([&](ConfigData& config) {
        _safeStringCopy(config.amapApiKey, apiKey.c_str(), sizeof(config.amapApiKey));
        _safeStringCopy(config.cityCode, cityCode.c_str(), sizeof(config.cityCode));
    });
}

bool UnifiedConfigManager::getConfigData(ConfigData& configData) {
    if (!_initialized) {
        return false;
    }
    
    if (_readFromEEPROM(configData)) {
        ConfigData defaultConfig;
        _getDefaultConfig(defaultConfig);
        
        // 填充空字段
        if (_isStringEmpty(configData.wifiSSID)) {
            _safeStringCopy(configData.wifiSSID, defaultConfig.wifiSSID, sizeof(configData.wifiSSID));
        }
        if (_isStringEmpty(configData.wifiPassword)) {
            _safeStringCopy(configData.wifiPassword, defaultConfig.wifiPassword, sizeof(configData.wifiPassword));
        }
        if (_isStringEmpty(configData.macAddress)) {
            _safeStringCopy(configData.macAddress, defaultConfig.macAddress, sizeof(configData.macAddress));
        }
        if (_isStringEmpty(configData.amapApiKey)) {
            _safeStringCopy(configData.amapApiKey, defaultConfig.amapApiKey, sizeof(configData.amapApiKey));
        }
        if (_isStringEmpty(configData.cityCode)) {
            _safeStringCopy(configData.cityCode, defaultConfig.cityCode, sizeof(configData.cityCode));
        }
        return true;
    }
    
    _getDefaultConfig(configData);
    return true;
}

bool UnifiedConfigManager::setConfigData(const ConfigData& configData) {
    return _initialized ? _writeToEEPROM(configData) : false;
}

bool UnifiedConfigManager::hasValidEEPROMConfig() {
    return _initialized ? _configManager->isValid() : false;
}

void UnifiedConfigManager::clearEEPROMConfig() {
    if (_initialized) {
        _configManager->clear();
        LOG_INFO("EEPROM configuration cleared");
    }
}

void UnifiedConfigManager::printCurrentConfig() {
    if (!_initialized) return;
    
    LogManager::printSeparator('=', 30);
    LogManager::info(F("当前配置信息"));
    LogManager::printSeparator('=', 30);
    
    LogManager::printKeyValue(F("WiFi SSID"), getWiFiSSID().c_str());
    LogManager::printKeyValue(F("WiFi 密码"), getWiFipassword().length() > 0 ? "***" : "未设置");
    LogManager::printKeyValue(F("MAC 地址"), getMacAddress().c_str());
    LogManager::printKeyValue(F("API 密钥"), getAmapApiKey().length() > 0 ? "***" : "未设置");
    LogManager::printKeyValue(F("城市代码"), getCityCode().c_str());
    LogManager::printKeyValue(F("EEPROM 配置"), hasValidEEPROMConfig() ? "有效" : "无效");
    
    LogManager::printSeparator('=', 30);
}

String UnifiedConfigManager::_getConfigValue(const char* eepromField, const char* defaultValue, size_t fieldOffset) {
    if (!_initialized) return String(defaultValue);
    
    ConfigData configData;
    if (_readFromEEPROM(configData)) {
        char* field = (char*)&configData + fieldOffset;
        
        // 调试：输出原始字段的十六进制数据
        Serial.print("DEBUG: Raw EEPROM field at offset ");
        Serial.print(fieldOffset);
        Serial.print(": ");
        for (int i = 0; i < 32 && field[i] != '\0'; i++) {
            Serial.print((uint8_t)field[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        
        if (!_isStringEmpty(field)) {
            String result = String(field);
            Serial.print("DEBUG: String result: '");
            Serial.print(result);
            Serial.println("'");
            return result;
        }
    }
    return String(defaultValue);
}

bool UnifiedConfigManager::_updateConfig(std::function<void(ConfigData&)> updateFunc) {
    if (!_initialized) return false;
    
    ConfigData configData;
    if (!_readFromEEPROM(configData)) {
        _getDefaultConfig(configData);
    }
    
    updateFunc(configData);
    return _writeToEEPROM(configData);
}

bool UnifiedConfigManager::_readFromEEPROM(ConfigData& configData) {
    return _initialized ? _configManager->read(configData) : false;
}

bool UnifiedConfigManager::_writeToEEPROM(const ConfigData& configData) {
    return _initialized ? _configManager->write(configData) : false;
}

void UnifiedConfigManager::_getDefaultConfig(ConfigData& configData) {
    memset(&configData, 0, sizeof(ConfigData));
    
    _safeStringCopy(configData.wifiSSID, DEFAULT_WIFI_SSID, sizeof(configData.wifiSSID));
    _safeStringCopy(configData.wifiPassword, DEFAULT_WIFI_PASSWORD, sizeof(configData.wifiPassword));
    _safeStringCopy(configData.macAddress, DEFAULT_MAC_ADDRESS, sizeof(configData.macAddress));
    _safeStringCopy(configData.amapApiKey, DEFAULT_AMAP_API_KEY, sizeof(configData.amapApiKey));
    _safeStringCopy(configData.cityCode, DEFAULT_CITY_CODE, sizeof(configData.cityCode));
    
    configData.temperature = 0.0f;
    configData.humidity = 0;
    configData.symbol = 0;
    configData.lastUpdateTime = 0;
}

bool UnifiedConfigManager::_isStringEmpty(const char* str) {
    if (str == nullptr) return true;
    
    for (size_t i = 0; i < 64; i++) {  // 限制检查长度，避免无限循环
        uint8_t c = (uint8_t)str[i];
        // 检查是否遇到字符串结束符
        if (c == 0) {
            return true;  // 空字符串
        }
        // 检查是否为EEPROM默认值（0xFF）
        if (c == 0xFF) {
            return true;  // EEPROM未初始化，视为空
        }
        // 检查是否为非空白字符
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            return false;  // 找到有效字符
        }
    }
    return true;  // 全是空白或超长
}

void UnifiedConfigManager::_safeStringCopy(char* dest, const char* src, size_t maxLen) {
    if (dest == nullptr || src == nullptr || maxLen == 0) return;
    
    // 清空目标缓冲区
    memset(dest, 0, maxLen);
    
    // 复制字符串，遇到0xFF或0时停止
    for (size_t i = 0; i < maxLen - 1; i++) {
        uint8_t c = (uint8_t)src[i];
        if (c == 0 || c == 0xFF) {
            break;  // 遇到结束符或EEPROM默认值
        }
        dest[i] = (char)c;
    }
    dest[maxLen - 1] = '\0';  // 确保字符串结束
}