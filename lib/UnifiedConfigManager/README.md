# UnifiedConfigManager 库

统一配置管理器，提供 EEPROM 和 config.h 的双重配置源管理，EEPROM 配置优先于 config.h 默认值。

## 功能特性

- EEPROM 配置优先级管理
- config.h 作为默认配置源
- 统一的配置接口
- WiFi 配置管理
- API 配置管理
- 配置有效性检查
- 配置数据持久化
- 安全的字符串操作

## 配置优先级

1. **EEPROM 存储的配置**（最高优先级）
2. **config.h 编译时配置**（默认值）

## 使用方法

### 基本初始化

```cpp
#include "UnifiedConfigManager.h"

// 创建统一配置管理器实例
UnifiedConfigManager configManager(512);  // EEPROM 大小 512 字节

void setup() {
    Serial.begin(115200);
    
    // 初始化配置管理器
    configManager.begin();
    
    // 检查是否有有效的 EEPROM 配置
    if (configManager.hasValidEEPROMConfig()) {
        Serial.println("使用 EEPROM 配置");
    } else {
        Serial.println("使用 config.h 默认配置");
    }
    
    // 显示当前配置
    configManager.printCurrentConfig();
}
```

### WiFi 配置管理

```cpp
void setupWiFiConfig() {
    // 获取 WiFi 配置
    String ssid = configManager.getWiFiSSID();
    String password = configManager.getWiFipassword();
    
    Serial.printf("WiFi SSID: %s\n", ssid.c_str());
    Serial.printf("WiFi 密码: %s\n", password.length() > 0 ? "***" : "未设置");
    
    // 设置新的 WiFi 配置
    if (configManager.setWiFiConfig("NewWiFiNetwork", "Newpassword123")) {
        Serial.println("WiFi 配置更新成功");
    } else {
        Serial.println("WiFi 配置更新失败");
    }
}
```

### API 配置管理

```cpp
void setupAPIConfig() {
    // 获取 API 配置
    String apiKey = configManager.getAmapApiKey();
    String cityCode = configManager.getCityCode();
    
    Serial.printf("API Key: %s\n", apiKey.length() > 0 ? "***" : "未设置");
    Serial.printf("城市代码: %s\n", cityCode.c_str());
    
    // 设置新的 API 配置
    if (configManager.setApiConfig("your_api_key_here", "110000")) {
        Serial.println("API 配置更新成功");
    } else {
        Serial.println("API 配置更新失败");
    }
}
```

### 完整配置操作

```cpp
void manageConfiguration() {
    // 获取完整配置数据
    ConfigData configData;
    if (configManager.getConfigData(configData)) {
        Serial.println("配置数据读取成功");
        
        // 修改配置数据
        strcpy(configData.wifi_ssid, "UpdatedSSID");
        strcpy(configData.wifi_password, "Updatedpassword");
        
        // 保存配置
        if (configManager.setConfigData(configData)) {
            Serial.println("配置数据保存成功");
        }
    }
    
    // 清除 EEPROM 配置（恢复到 config.h 默认值）
    // configManager.clearEEPROMConfig();
}
```

## API 参考

### 构造函数
- `UnifiedConfigManager(int eepromSize = 512)` - 创建统一配置管理器实例

### 初始化方法
- `void begin()` - 初始化配置管理器

### 配置获取方法
- `String getWiFiSSID()` - 获取 WiFi SSID
- `String getWiFipassword()` - 获取 WiFi 密码
- `String getMacAddress()` - 获取 MAC 地址
- `String getAmapApiKey()` - 获取高德地图 API 密钥
- `String getCityCode()` - 获取城市代码

### 配置设置方法
- `bool setWiFiConfig(const String& ssid, const String& password)` - 设置 WiFi 配置
- `bool setMacAddress(const String& macAddress)` - 设置 MAC 地址
- `bool setApiConfig(const String& apiKey, const String& cityCode)` - 设置 API 配置
- `bool setConfigData(const ConfigData& configData)` - 设置完整配置数据

### 配置管理方法
- `bool getConfigData(ConfigData& configData)` - 获取完整配置数据
- `bool hasValidEEPROMConfig()` - 检查是否有有效的 EEPROM 配置
- `void clearEEPROMConfig()` - 清除 EEPROM 配置
- `void printCurrentConfig()` - 打印当前配置

## 配置数据结构

ConfigData 结构体包含以下字段：

```cpp
struct ConfigData {
    char wifi_ssid[32];        // WiFi SSID
    char wifi_password[64]; // WiFi 密码
    char mac_address[18];      // MAC 地址
    char amap_api_key[64];     // 高德地图 API 密钥
    char city_code[8];         // 城市代码
    // 其他配置字段...
};
```

## 配置流程

### 1. 首次启动
```
启动 → 读取 EEPROM → 无配置 → 使用 config.h → 保存到 EEPROM
```

### 2. 正常启动
```
启动 → 读取 EEPROM → 有配置 → 使用 EEPROM 配置
```

### 3. 配置更新
```
用户设置 → 验证配置 → 保存到 EEPROM → 重启应用
```

### 4. 配置重置
```
清除 EEPROM → 重启 → 使用 config.h → 保存到 EEPROM
```

## 使用场景

### 1. 设备初始化

```cpp
void initializeDevice() {
    configManager.begin();
    
    if (!configManager.hasValidEEPROMConfig()) {
        Serial.println("首次启动，使用默认配置");
        enterConfigMode();
    } else {
        Serial.println("使用已保存的配置");
        startNormalOperation();
    }
}
```

### 2. 配置模式

```cpp
void enterConfigMode() {
    Serial.println("进入配置模式");
    
    // 等待用户输入配置
    while (!configComplete) {
        if (Serial.available()) {
            String input = Serial.readString();
            processConfigInput(input);
        }
        delay(100);
    }
    
    // 保存配置并重启
    configManager.printCurrentConfig();
    ESP.restart();
}
```

### 3. 配置验证

```cpp
bool validateConfiguration() {
    String ssid = configManager.getWiFiSSID();
    String apiKey = configManager.getAmapApiKey();
    
    if (ssid.length() == 0) {
        Serial.println("错误：WiFi SSID 未配置");
        return false;
    }
    
    if (apiKey.length() == 0) {
        Serial.println("错误：API 密钥未配置");
        return false;
    }
    
    return true;
}
```

## 错误处理

库提供了完善的错误处理机制：

1. **EEPROM 读取失败**：自动回退到 config.h 默认值
2. **配置验证失败**：拒绝无效配置
3. **字符串溢出保护**：防止缓冲区溢出
4. **空值检查**：避免空指针访问

## 安全特性

1. **字符串长度检查**：防止缓冲区溢出
2. **安全字符串复制**：使用安全的字符串操作函数
3. **配置验证**：确保配置数据的有效性
4. **默认值保护**：始终提供合理的默认配置

## 性能优化

1. **延迟初始化**：仅在需要时读取 EEPROM
2. **配置缓存**：避免重复读取 EEPROM
3. **批量更新**：支持一次性更新多个配置项

## 注意事项

1. **EEPROM 寿命**：避免频繁写入 EEPROM
2. **配置备份**：重要配置建议有备份机制
3. **版本兼容**：配置结构变更时需要考虑兼容性
4. **安全性**：敏感信息（如密码）需要适当保护

## 依赖库

- ConfigManager：基础配置管理
- EEPROM：配置数据存储
- Arduino：基础 Arduino 框架

## 配置示例

### config.h 示例

```cpp
#ifndef CONFIG_H
#define CONFIG_H

// 默认 WiFi 配置
#define DEFAULT_WIFI_SSID "WeWeather"
#define DEFAULT_WIFI_password ""

// 默认 API 配置
#define DEFAULT_AMAP_API_KEY ""
#define DEFAULT_CITY_CODE "110000"

// 默认 MAC 地址
#define DEFAULT_MAC_ADDRESS ""

#endif // CONFIG_H