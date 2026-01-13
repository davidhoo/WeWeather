# ConfigManager

通用配置管理器类，提供EEPROM配置存储功能，支持任意数据类型的配置存储和读取，包含校验和验证机制确保配置数据完整性。

## 特性

- 模板化设计，支持任意配置数据类型
- 自动校验和验证，确保配置数据完整性
- 简单易用的API接口
- 自动EEPROM初始化和管理

## 使用方法

### 基本用法

```cpp
#include "ConfigManager.h"

// 定义要存储的配置数据结构
struct MyConfig {
  int serverPort;
  float threshold;
  char deviceName[32];
  bool enableFeature;
};

// 创建配置管理器实例
ConfigManager<MyConfig> configManager(0, 512); // 地址0，EEPROM大小512字节

void setup() {
  // 初始化配置管理器
  configManager.begin();
  
  // 读取配置数据
  MyConfig config;
  if (configManager.read(config)) {
    Serial.println("配置数据读取成功");
    Serial.print("服务器端口: ");
    Serial.println(config.serverPort);
  } else {
    Serial.println("配置数据读取失败或校验和不匹配，使用默认配置");
    // 设置默认配置
    config.serverPort = 8080;
    config.threshold = 25.5;
    strcpy(config.deviceName, "WeWeather");
    config.enableFeature = true;
  }
  
  // 写入配置数据
  if (configManager.write(config)) {
    Serial.println("配置数据写入成功");
  }
  
  // 清除配置数据
  // configManager.clear();
}
```

### API 参考

#### 构造函数
```cpp
ConfigManager(int address = 0, int eepromSize = 512)
```
- `address`: EEPROM起始地址
- `eepromSize`: EEPROM总大小

#### 方法

- `void begin()`: 初始化配置管理器
- `bool read(T& data)`: 读取配置数据
- `bool write(const T& data)`: 写入配置数据
- `void clear()`: 清除存储的配置数据
- `bool isValid()`: 检查存储的配置数据是否有效
- `int getAddress() const`: 获取配置存储地址
- `void setAddress(int address)`: 设置配置存储地址
- `size_t getStorageSize() const`: 获取配置数据大小（包含校验和）

## 在WeatherManager中的使用

WeatherManager已经重构为使用ConfigManager来管理天气配置数据的存储：

```cpp
// 创建配置管理器
ConfigManager<WeatherStorageData>* _configManager;

// 在构造函数中初始化
_configManager = new ConfigManager<WeatherStorageData>(0, eepromSize);

// 读取天气配置数据
WeatherStorageData configData;
if (_configManager->read(configData)) {
  // 配置数据读取成功
}

// 写入天气配置数据
if (_configManager->write(configData)) {
  // 配置数据写入成功
}
```

## 设计优势

1. **语义明确**: "配置管理"比"存储管理"更准确地描述了功能用途
2. **代码复用性**: ConfigManager可以在其他模块中重复使用
3. **类型安全**: 模板化设计提供编译时类型检查
4. **数据完整性**: 自动校验和验证确保配置数据的可靠性
5. **易于扩展**: 其他模块可以轻松使用相同的配置管理机制

这种设计使得配置功能可以在其他模块中重复使用，提高了代码的可维护性和扩展性。