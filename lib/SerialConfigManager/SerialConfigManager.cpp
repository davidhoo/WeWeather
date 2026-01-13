#include "SerialConfigManager.h"
#include "../LogManager/LogManager.h"
#include <ESP8266WiFi.h>

/**
 * @brief 构造函数
 * @param configMgr 配置管理器指针
 */
SerialConfigManager::SerialConfigManager(ConfigManager<ConfigData>* configMgr) 
    : configManager(configMgr), isConfigMode(false) {
}

/**
 * @brief 析构函数
 */
SerialConfigManager::~SerialConfigManager() {
    // 析构函数实现
}

/**
 * @brief 初始化串口通信
 * ESP8266 ROM bootloader 使用 74880 波特率，保持一致便于查看启动信息
 * @param baudRate 波特率，默认使用配置文件中的值
 */
void SerialConfigManager::initializeSerial(uint32_t baudRate) {
    LogManager::begin(baudRate, LOG_INFO);
    LOG_INFO("Serial communication initialized");
}

/**
 * @brief 重新配置串口（用于配置模式）
 * 将RXD引脚从GPIO模式恢复为串口功能
 */
void SerialConfigManager::reconfigureSerial() {
    LOG_INFO("Reconfiguring serial for config mode...");
    
    // 重新配置RXD引脚为串口功能并重新初始化串口
    pinMode(RXD_PIN, INPUT);  // 移除上拉电阻，恢复串口功能
    
    // 重新初始化串口以确保RXD引脚正常工作
    Serial.end();
    delay(100);
    Serial.begin(SERIAL_BAUD_RATE);
    delay(100);
    
    LOG_INFO("RXD pin reconfigured and serial reinitialized");
}

/**
 * @brief 启动串口配置服务
 * 显示欢迎信息并准备接收命令
 */
void SerialConfigManager::startConfigService() {
    LOG_INFO("Starting serial configuration service...");
    
    isConfigMode = true;
    showWelcomeMessage();
    
    LOG_INFO("Serial configuration service started");
    LOG_INFO("Type 'help' for available commands");
}

/**
 * @brief 显示欢迎信息
 */
void SerialConfigManager::showWelcomeMessage() {
    Serial.println();
    Serial.println(F("=== WeWeather Serial Configuration ==="));
    Serial.println(F("Type 'help' for available commands"));
    printPrompt();
}

/**
 * @brief 打印命令提示符
 */
void SerialConfigManager::printPrompt() {
    Serial.print(F("> "));
}

/**
 * @brief 处理串口输入
 * 检查是否有串口数据可用并处理命令
 * @return true 如果处理了命令，false 如果没有数据
 */
bool SerialConfigManager::processInput() {
    if (!isConfigMode) {
        return false;
    }
    
    int available = Serial.available();
    if (available > 0) {
        LOG_INFO_F("Serial data available: %d bytes, processing command...", available);
        
        // 等待完整的命令行输入
        String command = "";
        while (Serial.available() > 0) {
            char c = Serial.read();
            if (c == '\n' || c == '\r') {
                break;
            }
            command += c;
            delay(1); // 短暂延时，确保接收完整
        }
        
        // 清除剩余的换行符
        while (Serial.available() > 0 && (Serial.peek() == '\n' || Serial.peek() == '\r')) {
            Serial.read();
        }
        
        command.trim(); // 去除首尾空白字符
        
        // 调试输出
        Serial.println(F("Received command: '") + command + F("' (length: ") + String(command.length()) + F(")"));
        if (command.length() > 0) {
            parseAndExecuteCommand(command);
        }
        
        printPrompt();
        return true;
    }
    
    return false;
}

/**
 * @brief 解析并执行命令
 * @param command 完整的命令字符串
 */
void SerialConfigManager::parseAndExecuteCommand(const String& command) {
    // 解析命令和参数
    int spaceIndex = command.indexOf(' ');
    String cmd = (spaceIndex > 0) ? command.substring(0, spaceIndex) : command;
    String args = (spaceIndex > 0) ? command.substring(spaceIndex + 1) : "";
    
    cmd.toLowerCase();
    
    // 调试输出
    Serial.println(F("Parsed command: '") + cmd + F("', args: '") + args + F("'"));
    
    // 执行命令
    if (cmd == "show") {
        showConfig();
    } else if (cmd == "set") {
        // 解析 set 命令的参数：set key value
        int argSpaceIndex = args.indexOf(' ');
        if (argSpaceIndex > 0) {
            String key = args.substring(0, argSpaceIndex);
            String value = args.substring(argSpaceIndex + 1);
            setConfig(key, value);
        } else {
            Serial.println(F("Usage: set <key> <value>"));
            Serial.println(F("Keys: ssid, password, apikey, citycode, mac"));
        }
    } else if (cmd == "clear") {
        clearConfig();
    } else if (cmd == "help") {
        showHelp();
    } else if (cmd == "exit") {
        exitConfigMode();
    } else {
        Serial.println(F("Unknown command: '") + cmd + F("'"));
        Serial.println(F("Type 'help' for available commands"));
    }
}

/**
 * @brief 检查配置键是否有效
 * @param key 配置键名
 * @return true 如果键有效，false 否则
 */
bool SerialConfigManager::isValidConfigKey(const String& key) {
    String lowerKey = key;
    lowerKey.toLowerCase();
    
    return (lowerKey == "ssid" || 
            lowerKey == "password" || 
            lowerKey == "apikey" || 
            lowerKey == "citycode" || 
            lowerKey == "mac");
}

/**
 * @brief 显示当前配置
 */
void SerialConfigManager::showConfig() {
    Serial.println(F("=== Current Configuration ==="));
    
    ConfigData config;
    if (configManager->read(config)) {
        Serial.println(F("SSID: ") + String(config.wifiSSID));
        Serial.println(F("Password: ") + String(config.wifiPassword));
        Serial.println(F("API Key: ") + String(config.amapApiKey));
        Serial.println(F("City Code: ") + String(config.cityCode));
        Serial.println(F("MAC Address: ") + String(config.macAddress));
    } else {
        Serial.println(F("No valid configuration found or failed to read"));
    }
    
    Serial.println(F("============================="));
}
/**
 * @brief 设置配置项
 * @param key 配置键名
 * @param value 配置值
 * @return true 如果设置成功，false 如果失败
 */
bool SerialConfigManager::setConfig(const String& key, const String& value) {
    String lowerKey = key;
    lowerKey.toLowerCase();
    
    if (!isValidConfigKey(lowerKey)) {
        Serial.println(F("Invalid key: ") + key);
        Serial.println(F("Valid keys: ssid, password, apikey, citycode, mac"));
        return false;
    }
    
    ConfigData config;
    // 尝试读取现有配置，如果失败则使用默认值
    if (!configManager->read(config)) {
        // 初始化为空配置
        memset(&config, 0, sizeof(config));
    }
    
    // 设置对应的配置项
    if (lowerKey == "ssid") {
        strncpy(config.wifiSSID, value.c_str(), sizeof(config.wifiSSID) - 1);
        config.wifiSSID[sizeof(config.wifiSSID) - 1] = '\0';
    } else if (lowerKey == "password") {
        strncpy(config.wifiPassword, value.c_str(), sizeof(config.wifiPassword) - 1);
        config.wifiPassword[sizeof(config.wifiPassword) - 1] = '\0';
    } else if (lowerKey == "apikey") {
        strncpy(config.amapApiKey, value.c_str(), sizeof(config.amapApiKey) - 1);
        config.amapApiKey[sizeof(config.amapApiKey) - 1] = '\0';
    } else if (lowerKey == "citycode") {
        strncpy(config.cityCode, value.c_str(), sizeof(config.cityCode) - 1);
        config.cityCode[sizeof(config.cityCode) - 1] = '\0';
    } else if (lowerKey == "mac") {
        strncpy(config.macAddress, value.c_str(), sizeof(config.macAddress) - 1);
        config.macAddress[sizeof(config.macAddress) - 1] = '\0';
    }
    
    Serial.println(F("Set ") + key + F(" = ") + value);
    
    // 直接写入EEPROM
    if (configManager->write(config)) {
        Serial.println(F("Configuration saved successfully"));
        return true;
    } else {
        Serial.println(F("Failed to save configuration"));
        return false;
    }
}

/**
 * @brief 清除配置
 * 只清除系统配置字段，保持天气数据不变
 * @return true 如果清除成功，false 如果失败
 */
bool SerialConfigManager::clearConfig() {
    ConfigData config;
    
    // 先读取现有配置，保持天气数据
    if (configManager->read(config)) {
        // 只清除系统配置字段，保持天气数据不变
        memset(config.wifiSSID, 0, sizeof(config.wifiSSID));
        memset(config.wifiPassword, 0, sizeof(config.wifiPassword));
        memset(config.amapApiKey, 0, sizeof(config.amapApiKey));
        memset(config.cityCode, 0, sizeof(config.cityCode));
        memset(config.macAddress, 0, sizeof(config.macAddress));
        
        // 写回配置，天气数据保持不变
        if (configManager->write(config)) {
            Serial.println(F("System configuration cleared (weather data preserved)"));
            return true;
        } else {
            Serial.println(F("Failed to clear configuration"));
            return false;
        }
    } else {
        Serial.println(F("No configuration found to clear"));
        return false;
    }
}

/**
 * @brief 显示帮助信息
 */
void SerialConfigManager::showHelp() {
    Serial.println(F("=== Available Commands ==="));
    Serial.println(F("show                    - Display current configuration"));
    Serial.println(F("set <key> <value>       - Set and save configuration value"));
    Serial.println(F("  Keys: ssid, password, apikey, citycode, mac"));
    Serial.println(F("clear                   - Clear all configuration"));
    Serial.println(F("help                    - Show this help message"));
    Serial.println(F("exit                    - Exit configuration mode (restart system)"));
    Serial.println(F("=========================="));
    Serial.println();
    Serial.println(F("Examples:"));
    Serial.println(F("  set ssid MyWiFi"));
    Serial.println(F("  set password myPassword"));
    Serial.println(F("  set apikey your_amap_api_key"));
    Serial.println(F("  set citycode 110108"));
    Serial.println(F("  set mac AA:BB:CC:DD:EE:FF"));
}

/**
 * @brief 退出配置模式
 * 重启系统以应用新配置
 */
void SerialConfigManager::exitConfigMode() {
    Serial.println(F("Exiting configuration mode..."));
    Serial.println(F("System will restart in 3 seconds..."));
    
    for (int i = 3; i > 0; i--) {
        Serial.println(String(i) + F("..."));
        delay(1000);
    }
    
    Serial.println(F("Restarting..."));
    Serial.flush();
    
    isConfigMode = false;
    ESP.restart();
}

/**
 * @brief 检查是否处于配置模式
 * @return true 如果处于配置模式，false 否则
 */
bool SerialConfigManager::isInConfigMode() const {
    return isConfigMode;
}

/**
 * @brief 设置配置模式状态
 * @param enabled 是否启用配置模式
 */
void SerialConfigManager::setConfigMode(bool enabled) {
    isConfigMode = enabled;
}