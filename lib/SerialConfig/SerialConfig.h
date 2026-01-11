#ifndef SERIAL_CONFIG_H
#define SERIAL_CONFIG_H

#include <Arduino.h>
#include "../ConfigManager/ConfigManager.h"

class ConfigSerial {
public:
  // 构造函数
  ConfigSerial(ConfigManager* configManager);
  
  // 初始化串口配置模式
  void begin(unsigned long baudRate = 74880);
  
  // 进入配置模式（阻塞式，等待配置完成或超时）
  bool enterConfigMode(unsigned long timeout = 30000);
  
  // 处理串口命令（非阻塞式，在 loop 中调用）
  void handleSerialInput();
  
  // 检查是否应该进入配置模式（检测特殊按键或条件）
  bool shouldEnterConfigMode();
  
  // 打印帮助信息
  void printHelp();
  
  // 打印当前配置
  void printCurrentConfig();
  
private:
  ConfigManager* _configManager;
  String _inputBuffer;
  bool _configMode;
  unsigned long _configModeStartTime;
  
  // 命令处理函数
  void _processCommand(const String& command);
  void _handleSetSSID(const String& value);
  void _handleSetPassword(const String& value);
  void _handleSetMacAddress(const String& value);
  void _handleSetApiKey(const String& value);
  void _handleSetCityCode(const String& value);
  void _handleSaveConfig();
  void _handleClearConfig();
  void _handleShowConfig();
  void _handleExit();
  
  // 辅助函数
  String _readLine();
  void _clearInputBuffer();
  bool _validateMacAddress(const String& mac);
};

#endif // SERIAL_CONFIG_H
