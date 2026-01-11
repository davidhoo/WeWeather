#ifndef WEB_CONFIG_H
#define WEB_CONFIG_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "../ConfigManager/ConfigManager.h"

class WebConfig {
public:
  // 构造函数
  WebConfig(ConfigManager* configManager);
  
  // 初始化 Web 配置模式
  void begin(const char* apSSID = "weweather", const char* apPassword = "");
  
  // 进入配置模式（阻塞式，等待配置完成或超时）
  bool enterConfigMode(unsigned long timeout = 300000); // 默认5分钟超时
  
  // 处理 Web 请求（非阻塞式，在 loop 中调用）
  void handleClient();
  
  // 停止 Web 服务器和 AP
  void stop();
  
  // 获取 AP IP 地址
  String getAPIP();
  
  // 获取 AP SSID
  String getAPSSID();
  
  // 检查是否配置完成
  bool isConfigured();

private:
  ConfigManager* _configManager;
  ESP8266WebServer* _server;
  String _apSSID;
  String _apPassword;
  bool _configMode;
  bool _configured;
  unsigned long _configModeStartTime;
  
  // Web 页面处理函数
  void _handleRoot();
  void _handleConfig();
  void _handleSave();
  void _handleStatus();
  void _handleNotFound();
  
  // HTML 页面生成函数
  String _generateConfigPage();
  String _generateSuccessPage();
  String _generateStatusPage();
  
  // 辅助函数
  String _htmlEncode(const String& str);
};

#endif // WEB_CONFIG_H
