#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>

// WiFi 配置结构体
struct WiFiConfig {
  char ssid[32];
  char password[64];
  unsigned long timeout;
  bool autoReconnect;
  int maxRetries;
  char macAddress[18];  // MAC地址字符串 (格式: "AA:BB:CC:DD:EE:FF")
  bool useMacAddress;   // 是否使用自定义MAC地址
  int failureCount;     // 连接失败计数
  bool configMode;      // 是否处于配网模式
};

// 配网模式配置
struct ConfigPortalConfig {
  char apName[32];      // AP名称
  char apPassword[64];  // AP密码（可选）
  IPAddress apIP;       // AP IP地址
  IPAddress gateway;    // 网关地址
  IPAddress subnet;     // 子网掩码
  int webServerPort;    // Web服务器端口
  unsigned long timeout; // 配网超时时间
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
  
  // 设置MAC地址
  void setMacAddress(const char* macAddress);
  
  // 获取当前MAC地址
  String getMacAddress();
  
  // 启用/禁用自定义MAC地址
  void enableMacAddress(bool enable);
  
  // 获取WiFi状态描述
  String getStatusString();
  
  // 打印当前配置信息
  void printConfig();
  
  // === 配网模式相关方法 ===
  
  // 启动配网模式
  bool startConfigPortal();
  
  // 启动配网模式（自定义AP名称）
  bool startConfigPortal(const char* apName);
  
  // 启动配网模式（完整配置）
  bool startConfigPortal(const ConfigPortalConfig& config);
  
  // 停止配网模式
  void stopConfigPortal();
  
  // 检查是否处于配网模式
  bool isConfigMode();
  
  // 处理配网模式的Web请求（需要在loop中调用）
  void handleConfigPortal();
  
  // 获取配网模式的AP信息
  String getConfigPortalSSID();
  String getConfigPortalIP();
  
  // 重置WiFi配置失败计数
  void resetFailureCount();
  
  // 获取WiFi配置失败计数
  int getFailureCount();
  
  // 增加WiFi配置失败计数
  void incrementFailureCount();
  
  // 检查是否应该进入配网模式
  bool shouldEnterConfigMode();
  
  // 保存WiFi配置到EEPROM
  bool saveConfigToEEPROM();
  
  // 从EEPROM加载WiFi配置
  bool loadConfigFromEEPROM();
  
  // 清除EEPROM中的WiFi配置
  void clearConfigFromEEPROM();
  
  // 智能连接（包含失败重试和自动配网逻辑）
  bool smartConnect();
  
private:
  WiFiConfig _config;
  bool _initialized;
  
  // 配网模式相关
  ConfigPortalConfig _portalConfig;
  ESP8266WebServer* _webServer;
  DNSServer* _dnsServer;
  bool _configPortalActive;
  unsigned long _configPortalStartTime;
  
  // EEPROM配置
  static const int EEPROM_SIZE = 512;
  static const int CONFIG_START_ADDRESS = 0;
  static const uint32_t CONFIG_VERSION = 0x12345678; // 配置版本标识
  
  // 内部辅助函数
  void _printNetworkInfo(int networkIndex);
  bool _waitForConnection(unsigned long timeout);
  void _copyString(char* dest, const char* src, size_t maxLen);
  bool _parseMacAddress(const char* macStr, uint8_t* macBytes);
  
  // 配网模式内部方法
  void _setupConfigPortal();
  void _setupWebServer();
  void _setupDNSServer();
  String _generateAPName();
  void _handleRoot();
  void _handleWiFiSave();
  void _handleNotFound();
  String _getConfigPageHTML();
  String _getSuccessPageHTML();
  String _getErrorPageHTML(const String& error);
  void _setDefaultPortalConfig();
  
  // EEPROM操作内部方法
  void _writeConfigToEEPROM(const WiFiConfig& config);
  bool _readConfigFromEEPROM(WiFiConfig& config);
  bool _isValidConfig(const WiFiConfig& config);
};

#endif // WIFI_MANAGER_H