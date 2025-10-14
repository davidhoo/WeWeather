#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>

// WiFi 配置结构体
struct WiFiConfig {
  char ssid[32];
  char password[64];
  unsigned long timeout;
  bool autoReconnect;
  int maxRetries;
};

class WiFiManager {
public:
  // 构造函数
  WiFiManager();
  
  // 初始化WiFi管理器（使用默认配置）
  void begin();
  
  // 初始化WiFi管理器（使用自定义配置）
  void begin(const WiFiConfig& config);
  
  // 设置默认WiFi配置
  void setDefaultConfig();
  
  // 设置WiFi凭据
  void setCredentials(const char* ssid, const char* password);
  
  // 设置完整配置
  void setConfig(const WiFiConfig& config);
  
  // 获取当前配置
  WiFiConfig getConfig() const;
  
  // 连接到WiFi网络
  bool connect(unsigned long timeout = 0); // 0表示使用配置中的超时时间
  
  // 扫描并连接到指定网络
  bool scanAndConnect(unsigned long timeout = 0);
  
  // 自动连接（使用配置中的参数）
  bool autoConnect();
  
  // 检查WiFi连接状态
  bool isConnected();
  
  // 断开WiFi连接
  void disconnect();
  
  // 获取本地IP地址
  String getLocalIP();
  
  // 获取信号强度
  int getRSSI();
  
  // 扫描可用网络
  int scanNetworks();
  
  // 获取扫描到的网络信息
  String getScannedSSID(int index);
  int getScannedRSSI(int index);
  bool isScannedNetworkSecure(int index);
  
  // 设置连接超时时间
  void setTimeout(unsigned long timeout);
  
  // 设置自动重连
  void setAutoReconnect(bool enable);
  
  // 设置最大重试次数
  void setMaxRetries(int retries);
  
  // 获取WiFi状态描述
  String getStatusString();
  
  // 打印当前配置信息
  void printConfig();
  
private:
  WiFiConfig _config;
  bool _initialized;
  
  // 内部辅助函数
  void _printNetworkInfo(int networkIndex);
  bool _waitForConnection(unsigned long timeout);
  void _copyString(char* dest, const char* src, size_t maxLen);
};

#endif // WIFI_MANAGER_H