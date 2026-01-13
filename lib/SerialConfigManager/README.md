# SerialConfigManager 库

串口配置管理库，提供通过串口接口进行设备配置的功能，支持命令行式的配置操作。

## 功能特性

- 串口通信初始化和重新配置
- 命令行式配置界面
- 配置参数的显示、设置和清除
- 配置模式管理
- 友好的用户交互界面
- 配置验证和错误处理

## 使用方法

### 基本初始化

```cpp
#include "SerialConfigManager.h"
#include "ConfigManager.h"

// 创建配置管理器
ConfigManager<ConfigData> configManager(0, 512);

// 创建串口配置管理器
SerialConfigManager serialConfig(&configManager);

void setup() {
    // 初始化串口通信
    serialConfig.initializeSerial(115200);
    
    // 启动配置服务
    serialConfig.startConfigService();
}

void loop() {
    // 处理串口输入
    if (serialConfig.processInput()) {
        // 处理了命令
    }
}
```

### 配置模式操作

```cpp
void enterConfigMode() {
    // 设置配置模式
    serialConfig.setConfigMode(true);
    
    // 显示欢迎信息
    serialConfig.startConfigService();
    
    // 在循环中处理输入
    while (serialConfig.isInConfigMode()) {
        serialConfig.processInput();
        delay(100);
    }
}
```

### 手动配置操作

```cpp
void manualConfigExample() {
    // 显示当前配置
    serialConfig.showConfig();
    
    // 设置配置项
    if (serialConfig.setConfig("ssid", "MyWiFi")) {
        Serial.println("WiFi SSID 设置成功");
    }
    
    if (serialConfig.setConfig("password", "password123")) {
        Serial.println("WiFi 密码设置成功");
    }
    
    // 清除配置
    if (serialConfig.clearConfig()) {
        Serial.println("配置清除成功");
    }
    
    // 显示帮助信息
    serialConfig.showHelp();
    
    // 退出配置模式
    serialConfig.exitConfigMode();
}
```

## 命令参考

### 基本命令

- `help` - 显示帮助信息
- `show` - 显示当前配置
- `exit` - 退出配置模式

### 配置命令

- `set <key> <value>` - 设置配置项
- `clear` - 清除所有配置

### 配置项

支持的配置项包括：

- `ssid` - WiFi 网络名称（对应 `wifiSSID` 字段）
- `password` - WiFi 密码（对应 `wifiPassword` 字段）
- `apikey` - 高德地图 API 密钥（对应 `amapApiKey` 字段）
- `citycode` - 城市代码（对应 `cityCode` 字段）
- `mac` - MAC 地址（对应 `macAddress` 字段）

## API 参考

### 构造函数
- `SerialConfigManager(ConfigManager<ConfigData>* configMgr)` - 创建串口配置管理器实例

### 初始化方法
- `void initializeSerial(uint32_t baudRate = SERIAL_BAUD_RATE)` - 初始化串口通信
- `void reconfigureSerial()` - 重新配置串口（用于配置模式）
- `void startConfigService()` - 启动串口配置服务

### 输入处理
- `bool processInput()` - 处理串口输入，返回是否处理了命令

### 配置操作
- `void showConfig()` - 显示当前配置
- `bool setConfig(const String& key, const String& value)` - 设置配置项
- `bool clearConfig()` - 清除配置（保持天气数据不变）

### 用户界面
- `void showHelp()` - 显示帮助信息
- `void exitConfigMode()` - 退出配置模式并重启系统

### 状态管理
- `bool isInConfigMode() const` - 检查是否处于配置模式
- `void setConfigMode(bool enabled)` - 设置配置模式状态

## 使用示例

### 完整配置流程

```cpp
#include "SerialConfigManager.h"

ConfigManager<ConfigData> configManager(0, 512);
SerialConfigManager serialConfig(&configManager);

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("按任意键进入配置模式...");
    
    // 等待用户输入
    unsigned long startTime = millis();
    while (millis() - startTime < 5000) {  // 5秒等待时间
        if (Serial.available()) {
            // 进入配置模式
            serialConfig.setConfigMode(true);
            serialConfig.startConfigService();
            
            // 配置循环
            while (serialConfig.isInConfigMode()) {
                serialConfig.processInput();
                delay(10);
            }
            break;
        }
        delay(100);
    }
    
    Serial.println("继续正常启动...");
}

void loop() {
    // 正常运行代码
}
```

### 配置验证

```cpp
bool validateConfiguration() {
    // 显示当前配置
    serialConfig.showConfig();
    
    // 验证必要的配置项
    ConfigData config;
    if (configManager.read(config)) {
        if (strlen(config.wifiSSID) == 0) {
            Serial.println("错误：WiFi SSID 未设置");
            return false;
        }
        
        if (strlen(config.amapApiKey) == 0) {
            Serial.println("错误：API 密钥未设置");
            return false;
        }
        
        Serial.println("配置验证通过");
        return true;
    }
    
    Serial.println("错误：无法读取配置");
    return false;
}
```

## 注意事项

1. **串口冲突**：在配置模式下，RXD 引脚会从 GPIO 模式切换为串口功能
2. **配置持久化**：所有配置更改都会自动保存到 EEPROM
3. **系统重启**：退出配置模式时会自动重启系统以应用新配置
4. **数据安全**：清除配置时不会删除天气数据，只清除系统配置
5. **命令格式**：所有命令不区分大小写，但配置键名区分大小写

## 错误处理

库提供了完善的错误处理机制：

- 无效的配置键名会被拒绝
- 配置值长度会进行验证
- 串口通信错误会被检测和报告
- 配置保存失败会显示错误信息

## 依赖库

- ConfigManager：配置数据管理
- Arduino：基础 Arduino 框架
- EEPROM：配置数据存储