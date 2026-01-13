#ifndef UNIFIED_CONFIG_MANAGER_H
#define UNIFIED_CONFIG_MANAGER_H

#include <Arduino.h>
#include "../ConfigManager/ConfigManager.h"
#include "../../config.h"

/**
 * 统一配置管理器
 * EEPROM 优先，config.h 作为默认值
 */
class UnifiedConfigManager {
public:
    UnifiedConfigManager(int eepromSize = 512);
    ~UnifiedConfigManager();
    
    void begin();
    
    // 配置获取
    String getWiFiSSID();
    String getWiFipassword();
    String getMacAddress();
    String getAmapApiKey();
    String getCityCode();
    
    // 配置设置
    bool setWiFiConfig(const String& ssid, const String& password);
    bool setMacAddress(const String& macAddress);
    bool setApiConfig(const String& apiKey, const String& cityCode);
    bool setConfigData(const ConfigData& configData);
    
    // 配置管理
    bool getConfigData(ConfigData& configData);
    bool hasValidEEPROMConfig();
    void clearEEPROMConfig();
    void printCurrentConfig();

private:
    ConfigManager<ConfigData>* _configManager;
    bool _initialized;
    
    // 通用配置获取方法
    String _getConfigValue(const char* eepromField, const char* defaultValue, size_t fieldOffset);
    bool _updateConfig(std::function<void(ConfigData&)> updateFunc);
    
    // 工具方法
    bool _readFromEEPROM(ConfigData& configData);
    bool _writeToEEPROM(const ConfigData& configData);
    void _getDefaultConfig(ConfigData& configData);
    bool _isStringEmpty(const char* str);
    void _safeStringCopy(char* dest, const char* src, size_t maxLen);
};

#endif // UNIFIED_CONFIG_MANAGER_H