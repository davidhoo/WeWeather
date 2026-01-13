#ifndef SERIAL_CONFIG_MANAGER_H
#define SERIAL_CONFIG_MANAGER_H

#include <Arduino.h>
#include "../../config.h"
#include "../ConfigManager/ConfigManager.h"

/**
 * @brief 串口配置管理类
 * 
 * 负责处理通过串口进行的设备配置功能，包括：
 * - 串口初始化和重新配置
 * - 串口命令解析和处理
 * - 配置参数的显示、设置和清除
 * - 配置模式的进入和退出
 */
class SerialConfigManager {
private:
    ConfigManager<ConfigData>* configManager;  // 配置管理器指针
    bool isConfigMode;                         // 是否处于配置模式
    
    // 私有方法
    void showWelcomeMessage();
    void parseAndExecuteCommand(const String& command);
    bool isValidConfigKey(const String& key);
    void printPrompt();
    
public:
    /**
     * @brief 构造函数
     * @param configMgr 配置管理器指针
     */
    SerialConfigManager(ConfigManager<ConfigData>* configMgr);
    
    /**
     * @brief 析构函数
     */
    ~SerialConfigManager();
    
    /**
     * @brief 初始化串口通信
     * @param baudRate 波特率，默认使用配置文件中的值
     */
    void initializeSerial(uint32_t baudRate = SERIAL_BAUD_RATE);
    
    /**
     * @brief 重新配置串口（用于配置模式）
     * 将RXD引脚从GPIO模式恢复为串口功能
     */
    void reconfigureSerial();
    
    /**
     * @brief 启动串口配置服务
     * 显示欢迎信息并准备接收命令
     */
    void startConfigService();
    
    /**
     * @brief 处理串口输入
     * 检查是否有串口数据可用并处理命令
     * @return true 如果处理了命令，false 如果没有数据
     */
    bool processInput();
    
    /**
     * @brief 显示当前配置
     */
    void showConfig();
    
    /**
     * @brief 设置配置项
     * @param key 配置键名
     * @param value 配置值
     * @return true 如果设置成功，false 如果失败
     */
    bool setConfig(const String& key, const String& value);
    
    /**
     * @brief 清除配置
     * 只清除系统配置字段，保持天气数据不变
     * @return true 如果清除成功，false 如果失败
     */
    bool clearConfig();
    
    /**
     * @brief 显示帮助信息
     */
    void showHelp();
    
    /**
     * @brief 退出配置模式
     * 重启系统以应用新配置
     */
    void exitConfigMode();
    
    /**
     * @brief 检查是否处于配置模式
     * @return true 如果处于配置模式，false 否则
     */
    bool isInConfigMode() const;
    
    /**
     * @brief 设置配置模式状态
     * @param enabled 是否启用配置模式
     */
    void setConfigMode(bool enabled);
};

#endif // SERIAL_CONFIG_MANAGER_H