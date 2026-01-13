#include "UnifiedConfigManager.h"
#include "../LogManager/LogManager.h"

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
        
        // 如果EEPROM中有数据，打印原始数据用于调试和分析
        if (hasValidEEPROMConfig()) {
            ConfigData rawData;
            if (_readFromEEPROM(rawData)) {
                LOG_INFO("=== EEPROM Raw Data Analysis ===");
                LOG_INFO_F("amapApiKey: '%s'", rawData.amapApiKey);
                LOG_INFO_F("cityCode: '%s'", rawData.cityCode);
                LOG_INFO_F("wifiSSID: '%s'", rawData.wifiSSID);
                LOG_INFO_F("wifiPassword: '%s'", rawData.wifiPassword);
                LOG_INFO_F("macAddress: '%s'", rawData.macAddress);
                
                // 分析数据是否错位
                String apiKey = String(rawData.amapApiKey);
                String cityCode = String(rawData.cityCode);
                String ssid = String(rawData.wifiSSID);
                String password = String(rawData.wifiPassword);
                
                // 检查是否需要数据迁移
                bool needsMigration = false;
                if (apiKey.indexOf("Sina") >= 0 || apiKey.indexOf("WiFi") >= 0) {
                    LOG_WARN("API Key appears to be WiFi SSID, data migration may be needed");
                    needsMigration = true;
                }
                if (cityCode.length() > 10 || cityCode.indexOf("urtheone") >= 0) {
                    LOG_WARN("City Code appears to be password, data migration may be needed");
                    needsMigration = true;
                }
                
                if (needsMigration) {
                    LOG_INFO("=== Suggested Data Migration ===");
                    LOG_INFO_F("Suggested SSID: '%s' (from amapApiKey)", rawData.amapApiKey);
                    LOG_INFO_F("Suggested password: '%s' (from cityCode)", rawData.cityCode);
                    LOG_INFO("Please use serial config to set correct values:");
                    LOG_INFO("  set ssid <your_wifi_ssid>");
                    LOG_INFO("  set password <your_wifi_password>");
                    LOG_INFO("  set apikey <your_amap_api_key>");
                    LOG_INFO("  set citycode <your_city_code>");
                }
                
                LOG_INFO("=== End Raw Data Analysis ===");
            }
        }
        
        // 打印当前配置信息用于调试
        printCurrentConfig();
    }
}

String UnifiedConfigManager::getWiFiSSID() {
    ConfigData configData;
    
    // 尝试从EEPROM读取
    if (_readFromEEPROM(configData) && !_isStringEmpty(configData.wifiSSID)) {
        LOG_INFO("Using WiFi SSID from EEPROM");
        return String(configData.wifiSSID);
    }
    
    // 使用config.h中的默认值
    LOG_INFO("Using WiFi SSID from config.h");
    return String(DEFAULT_WIFI_SSID);
}

String UnifiedConfigManager::getWiFipassword() {
    ConfigData configData;
    
    // 尝试从EEPROM读取
    if (_readFromEEPROM(configData) && !_isStringEmpty(configData.wifiPassword)) {
        LOG_INFO("Using WiFi password from EEPROM");
        return String(configData.wifiPassword);
    }
    
    // 使用config.h中的默认值
    LOG_INFO("Using WiFi password from config.h");
    return String(DEFAULT_WIFI_PASSWORD);
}

String UnifiedConfigManager::getMacAddress() {
    ConfigData configData;
    
    // 尝试从EEPROM读取
    if (_readFromEEPROM(configData) && !_isStringEmpty(configData.macAddress)) {
        LOG_INFO("Using MAC address from EEPROM");
        return String(configData.macAddress);
    }
    
    // 使用config.h中的默认值
    LOG_INFO("Using MAC address from config.h");
    return String(DEFAULT_MAC_ADDRESS);
}

String UnifiedConfigManager::getAmapApiKey() {
    ConfigData configData;
    
    // 尝试从EEPROM读取
    if (_readFromEEPROM(configData)) {
        LOG_INFO_F("DEBUG: Raw amapApiKey from EEPROM: '%s'", configData.amapApiKey);
        LOG_INFO_F("DEBUG: _isStringEmpty(configData.amapApiKey): %s", _isStringEmpty(configData.amapApiKey) ? "true" : "false");
        
        if (!_isStringEmpty(configData.amapApiKey)) {
            LOG_INFO("Using Amap API key from EEPROM");
            String apiKey = String(configData.amapApiKey);
            LOG_INFO_F("DEBUG: getAmapApiKey() returning: '%s'", apiKey.c_str());
            return apiKey;
        }
    }
    
    // 使用config.h中的默认值
    LOG_INFO("Using Amap API key from config.h");
    String defaultKey = String(DEFAULT_AMAP_API_KEY);
    LOG_INFO_F("DEBUG: getAmapApiKey() returning default: '%s'", defaultKey.c_str());
    return defaultKey;
}

String UnifiedConfigManager::getCityCode() {
    ConfigData configData;
    
    // 尝试从EEPROM读取
    if (_readFromEEPROM(configData) && !_isStringEmpty(configData.cityCode)) {
        String cityCode = String(configData.cityCode);
        
        // 检查城市代码是否合理（应该是数字，长度通常6位）
        bool isValidCityCode = true;
        if (cityCode.length() > 10 || cityCode.indexOf("urtheone") >= 0 ||
            cityCode.indexOf("password") >= 0 || cityCode.indexOf("password") >= 0) {
            isValidCityCode = false;
            LOG_WARN_F("City code appears to be invalid: '%s', using default", cityCode.c_str());
        }
        
        if (isValidCityCode) {
            LOG_INFO("Using city code from EEPROM");
            return cityCode;
        }
    }
    
    // 使用config.h中的默认值
    LOG_INFO("Using city code from config.h");
    return String(DEFAULT_CITY_CODE);
}

bool UnifiedConfigManager::setWiFiConfig(const String& ssid, const String& password) {
    if (!_initialized) {
        LOG_ERROR("UnifiedConfigManager not initialized");
        return false;
    }
    
    ConfigData configData;
    
    // 先读取现有配置，如果没有则使用默认配置
    if (!_readFromEEPROM(configData)) {
        _getDefaultConfig(configData);
    }
    
    // 更新WiFi配置
    _safeStringCopy(configData.wifiSSID, ssid.c_str(), sizeof(configData.wifiSSID));
    _safeStringCopy(configData.wifiPassword, password.c_str(), sizeof(configData.wifiPassword));
    
    // 写入EEPROM
    bool success = _writeToEEPROM(configData);
    if (success) {
        LOG_INFO("WiFi configuration saved to EEPROM");
    } else {
        LOG_ERROR("Failed to save WiFi configuration to EEPROM");
    }
    
    return success;
}

bool UnifiedConfigManager::setMacAddress(const String& macAddress) {
    if (!_initialized) {
        LOG_ERROR("UnifiedConfigManager not initialized");
        return false;
    }
    
    ConfigData configData;
    
    // 先读取现有配置，如果没有则使用默认配置
    if (!_readFromEEPROM(configData)) {
        _getDefaultConfig(configData);
    }
    
    // 更新MAC地址
    _safeStringCopy(configData.macAddress, macAddress.c_str(), sizeof(configData.macAddress));
    
    // 写入EEPROM
    bool success = _writeToEEPROM(configData);
    if (success) {
        LOG_INFO("MAC address saved to EEPROM");
    } else {
        LOG_ERROR("Failed to save MAC address to EEPROM");
    }
    
    return success;
}

bool UnifiedConfigManager::setApiConfig(const String& apiKey, const String& cityCode) {
    if (!_initialized) {
        LOG_ERROR("UnifiedConfigManager not initialized");
        return false;
    }
    
    ConfigData configData;
    
    // 先读取现有配置，如果没有则使用默认配置
    if (!_readFromEEPROM(configData)) {
        _getDefaultConfig(configData);
    }
    
    // 更新API配置
    _safeStringCopy(configData.amapApiKey, apiKey.c_str(), sizeof(configData.amapApiKey));
    _safeStringCopy(configData.cityCode, cityCode.c_str(), sizeof(configData.cityCode));
    
    // 写入EEPROM
    bool success = _writeToEEPROM(configData);
    if (success) {
        LOG_INFO("API configuration saved to EEPROM");
    } else {
        LOG_ERROR("Failed to save API configuration to EEPROM");
    }
    
    return success;
}

bool UnifiedConfigManager::getConfigData(ConfigData& configData) {
    if (!_initialized) {
        LOG_ERROR("UnifiedConfigManager not initialized");
        return false;
    }
    
    // 尝试从EEPROM读取完整配置
    if (_readFromEEPROM(configData)) {
        // 检查每个字段，如果为空则使用默认值
        ConfigData defaultConfig;
        _getDefaultConfig(defaultConfig);
        
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
    
    // 如果EEPROM中没有配置，使用默认配置
    _getDefaultConfig(configData);
    return true;
}

bool UnifiedConfigManager::setConfigData(const ConfigData& configData) {
    if (!_initialized) {
        LOG_ERROR("UnifiedConfigManager not initialized");
        return false;
    }
    
    return _writeToEEPROM(configData);
}

bool UnifiedConfigManager::hasValidEEPROMConfig() {
    if (!_initialized) {
        return false;
    }
    
    return _configManager->isValid();
}

void UnifiedConfigManager::clearEEPROMConfig() {
    if (!_initialized) {
        LOG_ERROR("UnifiedConfigManager not initialized");
        return;
    }
    
    _configManager->clear();
    LOG_INFO("EEPROM configuration cleared");
}

void UnifiedConfigManager::printCurrentConfig() {
    if (!_initialized) {
        LOG_ERROR("UnifiedConfigManager not initialized");
        return;
    }
    
    LogManager::printSeparator('=', 30);
    LogManager::info(F("当前配置信息"));
    LogManager::printSeparator('=', 30);
    
    String ssid = getWiFiSSID();
    String password = getWiFipassword();
    String mac = getMacAddress();
    String apiKey = getAmapApiKey();
    String cityCode = getCityCode();
    
    LogManager::printKeyValue(F("WiFi SSID"), ssid.c_str());
    LogManager::printKeyValue(F("WiFi 密码"), password.length() > 0 ? "***" : "未设置");
    LogManager::printKeyValue(F("MAC 地址"), mac.c_str());
    LogManager::printKeyValue(F("API 密钥"), apiKey.length() > 0 ? "***" : "未设置");
    LogManager::printKeyValue(F("城市代码"), cityCode.c_str());
    LogManager::printKeyValue(F("EEPROM 配置"), hasValidEEPROMConfig() ? "有效" : "无效");
    
    LogManager::printSeparator('=', 30);
}

bool UnifiedConfigManager::_readFromEEPROM(ConfigData& configData) {
    if (!_initialized) {
        return false;
    }
    
    return _configManager->read(configData);
}

bool UnifiedConfigManager::_writeToEEPROM(const ConfigData& configData) {
    if (!_initialized) {
        return false;
    }
    
    return _configManager->write(configData);
}

void UnifiedConfigManager::_getDefaultConfig(ConfigData& configData) {
    // 清零结构体
    memset(&configData, 0, sizeof(ConfigData));
    
    // 设置默认值（来自config.h）
    _safeStringCopy(configData.wifiSSID, DEFAULT_WIFI_SSID, sizeof(configData.wifiSSID));
    _safeStringCopy(configData.wifiPassword, DEFAULT_WIFI_PASSWORD, sizeof(configData.wifiPassword));
    _safeStringCopy(configData.macAddress, DEFAULT_MAC_ADDRESS, sizeof(configData.macAddress));
    _safeStringCopy(configData.amapApiKey, DEFAULT_AMAP_API_KEY, sizeof(configData.amapApiKey));
    _safeStringCopy(configData.cityCode, DEFAULT_CITY_CODE, sizeof(configData.cityCode));
    
    // 其他字段保持为0/空
    configData.temperature = 0.0f;
    configData.humidity = 0;
    configData.symbol = 0;
    configData.lastUpdateTime = 0;
}

bool UnifiedConfigManager::_isStringEmpty(const char* str) {
    if (str == nullptr) {
        return true;
    }
    
    // 检查是否为空字符串或只包含空白字符
    while (*str) {
        if (*str != ' ' && *str != '\t' && *str != '\n' && *str != '\r') {
            return false;
        }
        str++;
    }
    
    return true;
}

void UnifiedConfigManager::_safeStringCopy(char* dest, const char* src, size_t maxLen) {
    if (dest == nullptr || src == nullptr || maxLen == 0) {
        return;
    }
    
    strncpy(dest, src, maxLen - 1);
    dest[maxLen - 1] = '\0';  // 确保字符串以null结尾
}