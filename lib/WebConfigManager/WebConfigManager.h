#ifndef WEB_CONFIG_MANAGER_H
#define WEB_CONFIG_MANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "../../config.h"
#include "../ConfigManager/ConfigManager.h"

/**
 * @brief Web配置管理类
 *
 * 负责处理通过Web界面进行的设备配置功能，包括：
 * - Web服务器初始化和路由处理
 * - 配置参数的Web界面显示、设置和保存
 * - 配置请求的处理
 */
class WebConfigManager {
private:
    ConfigManager<ConfigData>* configManager;  // 配置管理器指针
    ESP8266WebServer* webServer;               // Web服务器指针
    bool isConfigMode;                         // 是否处于配置模式
    
    // 私有方法
    void setupWebRoutes();                     // 设置Web路由
    void handleRoot();                         // 处理根路径请求
    void handleConfig();                       // 处理配置页面请求
    void handleSave();                         // 处理保存配置请求
    void handleExit();                         // 处理退出配置请求
    void handleNotFound();                     // 处理404请求
    String generateConfigPage();               // 生成配置页面HTML
    String generateSuccessPage();              // 生成成功页面HTML
    String generateErrorPage();                // 生成错误页面HTML
    String generateExitPage();                 // 生成退出页面HTML
    
public:
    /**
     * @brief 构造函数
     * @param configMgr 配置管理器指针
     */
    WebConfigManager(ConfigManager<ConfigData>* configMgr);
    
    /**
     * @brief 析构函数
     */
    ~WebConfigManager();
    
    /**
     * @brief 启动Web服务器
     * @param port 服务器端口，默认80
     * @return true 如果启动成功，false 如果失败
     */
    bool startWebServer(int port = 80);
    
    /**
     * @brief 停止Web服务器
     */
    void stopWebServer();
    
    /**
     * @brief 启动Web配置服务
     * 启动Web服务器，准备接收配置请求
     * @return true 如果启动成功，false 如果失败
     */
    bool startConfigService();
    
    /**
     * @brief 处理Web请求
     * 处理客户端的Web请求，需要在主循环中调用
     */
    void handleClient();
    
    /**
     * @brief 退出配置模式
     * 停止Web服务器，重启系统以应用新配置
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

#endif // WEB_CONFIG_MANAGER_H