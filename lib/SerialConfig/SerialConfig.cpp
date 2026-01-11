#include "SerialConfig.h"

ConfigSerial::ConfigSerial(ConfigManager* configManager) {
  _configManager = configManager;
  _configMode = false;
  _configModeStartTime = 0;
}

void ConfigSerial::begin(unsigned long baudRate) {
  Serial.begin(baudRate);
  Serial.println("\n=== SerialConfig 初始化完成 ===");
  Serial.println("提示: 启动后 5 秒内发送 'config' 进入配置模式");
}

bool ConfigSerial::enterConfigMode(unsigned long timeout) {
  _configMode = true;
  _configModeStartTime = millis();
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║     进入串口配置模式                   ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println();
  printHelp();
  
  // 阻塞式等待配置
  while (_configMode && (millis() - _configModeStartTime < timeout)) {
    handleSerialInput();
    delay(10);
  }
  
  if (_configMode) {
    Serial.println("\n配置模式超时，退出配置模式");
    _configMode = false;
    return false;
  }
  
  return true;
}

void ConfigSerial::handleSerialInput() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    
    if (c == '\n' || c == '\r') {
      if (_inputBuffer.length() > 0) {
        _processCommand(_inputBuffer);
        _inputBuffer = "";
      }
    } else {
      _inputBuffer += c;
    }
  }
}

bool ConfigSerial::shouldEnterConfigMode() {
  // 检查启动后 5 秒内是否收到 "config" 命令
  unsigned long startTime = millis();
  
  Serial.println("等待配置命令... (10秒内发送 'config' 进入配置模式)");
  
  while (millis() - startTime < 10000) {
    if (Serial.available() > 0) {
      String input = _readLine();
      input.trim();
      
      if (input.equalsIgnoreCase("config")) {
        return true;
      }
    }
    delay(100);
  }
  
  return false;
}

void ConfigSerial::printHelp() {
  Serial.println("可用命令:");
  Serial.println("  set ssid <SSID>           - 设置 WiFi SSID");
  Serial.println("  set password <password>     - 设置 WiFi 密码");
  Serial.println("  set mac <MAC>             - 设置 MAC 地址 (格式: AA:BB:CC:DD:EE:FF)");
  Serial.println("  set apikey <KEY>          - 设置高德地图 API Key");
  Serial.println("  set citycode <CODE>       - 设置城市代码");
  Serial.println("  save                      - 保存配置到 EEPROM");
  Serial.println("  show                      - 显示当前配置");
  Serial.println("  clear                     - 清除配置");
  Serial.println("  help                      - 显示帮助信息");
  Serial.println("  exit                      - 退出配置模式");
  Serial.println();
}

void ConfigSerial::printCurrentConfig() {
  if (_configManager) {
    _configManager->printConfig();
  }
}

void ConfigSerial::_processCommand(const String& command) {
  String cmd = command;
  cmd.trim();
  
  // 转换为小写以便比较
  String cmdLower = cmd;
  cmdLower.toLowerCase();
  
  Serial.println("> " + cmd);
  
  if (cmdLower.startsWith("set ssid ")) {
    _handleSetSSID(cmd.substring(9));
  } 
  else if (cmdLower.startsWith("set password ") || cmdLower.startsWith("set password ")) {
    int startPos = cmdLower.startsWith("set password ") ? 13 : 13;
    _handleSetPassword(cmd.substring(startPos));
  }
  else if (cmdLower.startsWith("set mac ")) {
    _handleSetMacAddress(cmd.substring(8));
  }
  else if (cmdLower.startsWith("set apikey ")) {
    _handleSetApiKey(cmd.substring(11));
  }
  else if (cmdLower.startsWith("set citycode ")) {
    _handleSetCityCode(cmd.substring(13));
  }
  else if (cmdLower == "save") {
    _handleSaveConfig();
  }
  else if (cmdLower == "show") {
    _handleShowConfig();
  }
  else if (cmdLower == "clear") {
    _handleClearConfig();
  }
  else if (cmdLower == "help") {
    printHelp();
  }
  else if (cmdLower == "exit") {
    _handleExit();
  }
  else {
    Serial.println("错误: 未知命令 '" + cmd + "'");
    Serial.println("输入 'help' 查看可用命令");
  }
  
  Serial.println();
}

void ConfigSerial::_handleSetSSID(const String& value) {
  String ssid = value;
  ssid.trim();
  
  if (ssid.length() == 0) {
    Serial.println("错误: SSID 不能为空");
    return;
  }
  
  if (ssid.length() > 31) {
    Serial.println("错误: SSID 长度不能超过 31 个字符");
    return;
  }
  
  _configManager->setSSID(ssid.c_str());
  Serial.println("✓ SSID 已设置");
}

void ConfigSerial::_handleSetPassword(const String& value) {
  String password = value;
  password.trim();
  
  if (password.length() == 0) {
    Serial.println("错误: 密码不能为空");
    return;
  }
  
  if (password.length() > 63) {
    Serial.println("错误: 密码长度不能超过 63 个字符");
    return;
  }
  
  _configManager->setPassword(password.c_str());
  Serial.println("✓ 密码已设置");
}

void ConfigSerial::_handleSetMacAddress(const String& value) {
  String mac = value;
  mac.trim();
  
  if (!_validateMacAddress(mac)) {
    Serial.println("错误: MAC 地址格式无效");
    Serial.println("正确格式: AA:BB:CC:DD:EE:FF");
    return;
  }
  
  _configManager->setMacAddress(mac.c_str());
  Serial.println("✓ MAC 地址已设置");
}

void ConfigSerial::_handleSetApiKey(const String& value) {
  String apiKey = value;
  apiKey.trim();
  
  if (apiKey.length() == 0) {
    Serial.println("错误: API Key 不能为空");
    return;
  }
  
  if (apiKey.length() > 63) {
    Serial.println("错误: API Key 长度不能超过 63 个字符");
    return;
  }
  
  _configManager->setAmapApiKey(apiKey.c_str());
  Serial.println("✓ 高德地图 API Key 已设置");
}

void ConfigSerial::_handleSetCityCode(const String& value) {
  String cityCode = value;
  cityCode.trim();
  
  if (cityCode.length() == 0) {
    Serial.println("错误: 城市代码不能为空");
    return;
  }
  
  if (cityCode.length() > 15) {
    Serial.println("错误: 城市代码长度不能超过 15 个字符");
    return;
  }
  
  _configManager->setCityCode(cityCode.c_str());
  Serial.println("✓ 城市代码已设置");
}

void ConfigSerial::_handleSaveConfig() {
  DeviceConfig config = _configManager->getConfig();
  
  Serial.println("正在保存配置到 EEPROM...");
  
  if (_configManager->saveConfig(config)) {
    Serial.println("✓ 配置已成功保存到 EEPROM");
    
    // 额外延迟确保写入完成
    delay(200);
    
    // 再次验证配置
    DeviceConfig verifyConfig;
    if (_configManager->loadConfig(verifyConfig)) {
      Serial.println("✓ 配置验证成功，数据已持久化");
    } else {
      Serial.println("⚠ 警告: 配置验证失败，请重新保存");
    }
  } else {
    Serial.println("✗ 保存配置失败");
    Serial.println("提示: 请检查配置是否完整，然后重试");
  }
}

void ConfigSerial::_handleClearConfig() {
  Serial.println("警告: 即将清除所有配置!");
  Serial.println("输入 'yes' 确认清除: ");
  
  // 等待确认
  unsigned long startTime = millis();
  String confirmation = "";
  
  while (millis() - startTime < 10000) {
    if (Serial.available() > 0) {
      confirmation = _readLine();
      confirmation.trim();
      break;
    }
    delay(10);
  }
  
  if (confirmation.equalsIgnoreCase("yes")) {
    _configManager->clearConfig();
    Serial.println("✓ 配置已清除");
  } else {
    Serial.println("取消清除操作");
  }
}

void ConfigSerial::_handleShowConfig() {
  printCurrentConfig();
}

void ConfigSerial::_handleExit() {
  Serial.println("退出配置模式");
  _configMode = false;
}

String ConfigSerial::_readLine() {
  String line = "";
  
  while (Serial.available() > 0) {
    char c = Serial.read();
    
    if (c == '\n' || c == '\r') {
      if (line.length() > 0) {
        break;
      }
    } else {
      line += c;
    }
    
    delay(2);
  }
  
  return line;
}

void ConfigSerial::_clearInputBuffer() {
  while (Serial.available() > 0) {
    Serial.read();
  }
}

bool ConfigSerial::_validateMacAddress(const String& mac) {
  // 检查长度
  if (mac.length() != 17) {
    return false;
  }
  
  // 检查格式 (AA:BB:CC:DD:EE:FF)
  for (int i = 0; i < 17; i++) {
    if (i % 3 == 2) {
      // 应该是冒号
      if (mac[i] != ':') {
        return false;
      }
    } else {
      // 应该是十六进制字符
      char c = mac[i];
      if (!((c >= '0' && c <= '9') || 
            (c >= 'A' && c <= 'F') || 
            (c >= 'a' && c <= 'f'))) {
        return false;
      }
    }
  }
  
  return true;
}
