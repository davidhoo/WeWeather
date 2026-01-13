#ifndef UNIFIED_CONFIG_MANAGER_H
#define UNIFIED_CONFIG_MANAGER_H

#include <Arduino.h>
#include "../ConfigManager/ConfigManager.h"
#include "../../config.h"

/**
 * 统一配置管理器
 * 实现 EEPROM 优先，config.h 作为默认值的配置管理逻辑
 * 
 * 优先级规则：
 * 1. 如果 EEPROM 中有有效配置，使用 EEPROM 中的配置
 * 2. 如果 EEPROM 中没有配置或配置无效，使用 config.h 中的默认配置
 */
class UnifiedConfigManager {
public:
    /**
     * 构造函数
     * @param eepromSize EEPROM总大小
     */
    UnifiedConfigManager(int eepromSize = 512);
    
    /**
     * 析构函数
     */
    ~UnifiedConfigManager();
    
    /**
     * 初始化配置管理器
     */
    void begin();
    
    /**
     * 获取WiFi SSID
     * @return WiFi SSID字符串
     */
    String getWiFiSSID();
    
    /**
     * 获取WiFi密码
     * @return WiFi密码字符串
     */
    String getWiFipassword();
    
    /**
     * 获取MAC地址
     * @return MAC地址字符串
     */
    String getMacAddress();
    
    /**
     * 获取高德地图API密钥
     * @return API密钥字符串
     */
    String getAmapApiKey();
    
    /**
     * 获取城市代码
     * @return 城市代码字符串
     */
    String getCityCode();
    
    /**
     * 设置WiFi配置
     * @param ssid WiFi SSID
     * @param password WiFi密码
     * @return 是否设置成功
     */
    bool setWiFiConfig(const String& ssid, const String& password);
    
    /**
     * 设置MAC地址
     * @param macAddress MAC地址
     * @return 是否设置成功
     */
    bool setMacAddress(const String& macAddress);
    
    /**
     * 设置API配置
     * @param apiKey 高德地图API密钥
     * @param cityCode 城市代码
     * @return 是否设置成功
     */
    bool setApiConfig(const String& apiKey, const String& cityCode);
    
    /**
     * 获取完整的配置数据
     * @param configData 输出参数，配置数据
     * @return 是否获取成功
     */
    bool getConfigData(ConfigData& configData);
    
    /**
     * 设置完整的配置数据
     * @param configData 配置数据
     * @return 是否设置成功
     */
    bool setConfigData(const ConfigData& configData);
    
    /**
     * 检查EEPROM中是否有有效配置
     * @return 是否有有效配置
     */
    bool hasValidEEPROMConfig();
    
    /**
     * 清除EEPROM配置
     */
    void clearEEPROMConfig();
    
    /**
     * 打印当前配置信息（用于调试）
     */
    void printCurrentConfig();

private:
    ConfigManager<ConfigData>* _configManager;
    bool _initialized;
    
    /**
     * 从EEPROM读取配置数据
     * @param configData 输出参数，配置数据
     * @return 是否读取成功
     */
    bool _readFromEEPROM(ConfigData& configData);
    
    /**
     * 将配置数据写入EEPROM
     * @param configData 配置数据
     * @return 是否写入成功
     */
    bool _writeToEEPROM(const ConfigData& configData);
    
    /**
     * 获取默认配置（来自config.h）
     * @param configData 输出参数，默认配置数据
     */
    void _getDefaultConfig(ConfigData& configData);
    
    /**
     * 检查字符串是否为空或只包含空白字符
     * @param str 要检查的字符串
     * @return 是否为空
     */
    bool _isStringEmpty(const char* str);
    
    /**
     * 安全地复制字符串
     * @param dest 目标缓冲区
     * @param src 源字符串
     * @param maxLen 最大长度
     */
    void _safeStringCopy(char* dest, const char* src, size_t maxLen);
};

#endif // UNIFIED_CONFIG_MANAGER_H