# 串口配置功能开发总结

## 分支信息
- **分支名称**: `feature/serial-config`
- **基于分支**: main
- **提交哈希**: d8d148b

## 功能概述

成功实现了串口配置模式，允许用户通过串口命令配置设备参数并保存到 EEPROM。

## 新增文件

### 1. ConfigManager 库
- **位置**: `lib/ConfigManager/`
- **文件**:
  - [`ConfigManager.h`](lib/ConfigManager/ConfigManager.h) - 配置管理器头文件
  - [`ConfigManager.cpp`](lib/ConfigManager/ConfigManager.cpp) - 配置管理器实现

**功能**:
- 管理 EEPROM 配置的读写
- 配置数据校验（使用校验和）
- 支持配置项：SSID、WiFi 密码、MAC 地址、高德地图 API Key、城市代码

### 2. ConfigSerial 库
- **位置**: `lib/SerialConfig/`
- **文件**:
  - [`SerialConfig.h`](lib/SerialConfig/SerialConfig.h) - 串口配置处理器头文件
  - [`SerialConfig.cpp`](lib/SerialConfig/SerialConfig.cpp) - 串口配置处理器实现
  - [`README.md`](lib/SerialConfig/README.md) - 使用说明文档

**功能**:
- 处理串口命令输入
- 提供交互式配置界面
- 支持命令：set、save、show、clear、help、exit

### 3. 修改的文件
- [`src/main.cpp`](src/main.cpp) - 集成串口配置功能到主程序

## 核心特性

### 1. 配置项支持
- ✅ WiFi SSID
- ✅ WiFi 密码
- ✅ 自定义 MAC 地址
- ✅ 高德地图 API Key
- ✅ 城市代码

### 2. 数据持久化
- 使用 EEPROM 存储配置
- 配置数据包含校验和，确保数据完整性
- 启动时自动加载并验证配置

### 3. 配置优先级
1. **EEPROM 配置**（优先）- 用户通过串口设置的配置
2. **默认配置**（备用）- 代码中硬编码的默认值

### 4. 用户交互
- 启动后 5 秒内发送 `config` 进入配置模式
- 配置模式超时时间：60 秒
- 友好的命令行界面
- 实时反馈和错误提示

## 使用方法

### 进入配置模式
```bash
# 设备启动后 5 秒内发送
config
```

### 配置示例
```bash
> set ssid MyWiFi
✓ SSID 已设置

> set password Mypassword123
✓ 密码已设置

> set mac AA:BB:CC:DD:EE:FF
✓ MAC 地址已设置

> set apikey your_api_key_here
✓ 高德地图 API Key 已设置

> set citycode 110108
✓ 城市代码已设置

> save
✓ 配置已成功保存到 EEPROM

> exit
退出配置模式
```

## 技术实现

### ConfigManager 类
```cpp
class ConfigManager {
  - begin()                    // 初始化并加载配置
  - saveConfig()               // 保存配置到 EEPROM
  - loadConfig()               // 从 EEPROM 加载配置
  - validateConfig()           // 验证配置完整性
  - clearConfig()              // 清除配置
  - setSSID/setPassword/...    // 设置各项配置
}
```

### ConfigSerial 类
```cpp
class ConfigSerial {
  - begin()                    // 初始化串口配置
  - shouldEnterConfigMode()    // 检查是否进入配置模式
  - enterConfigMode()          // 进入配置模式（阻塞式）
  - handleSerialInput()        // 处理串口输入
  - printHelp()                // 打印帮助信息
}
```

### EEPROM 数据结构
```cpp
struct DeviceConfig {
  char ssid[32];              // WiFi SSID
  char password[64];          // WiFi 密码
  char macAddress[18];        // MAC地址
  char amapApiKey[64];        // 高德地图 API Key
  char cityCode[16];          // 城市代码
  bool isConfigured;          // 配置标志
  uint16_t checksum;          // 校验和
};
```

## 编译结果

✅ **编译成功**
- RAM 使用: 51.3% (41988 / 81920 bytes)
- Flash 使用: 44.8% (467439 / 1044464 bytes)
- 无错误，仅有警告（来自第三方库）

## 注意事项

1. **类名冲突**: 原计划使用 `SerialConfig` 类名，但与 ESP8266 核心库的枚举类型冲突，改为 `ConfigSerial`

2. **字符串长度限制**:
   - SSID: 最多 31 个字符
   - 密码: 最多 63 个字符
   - MAC 地址: 固定 17 个字符（格式：AA:BB:CC:DD:EE:FF）
   - API Key: 最多 63 个字符
   - 城市代码: 最多 15 个字符

3. **MAC 地址格式**: 必须使用标准格式 `AA:BB:CC:DD:EE:FF`

4. **配置保存**: 设置完成后必须执行 `save` 命令才会保存到 EEPROM

## 后续建议

1. **增强功能**:
   - 添加配置导入/导出功能
   - 支持通过 Web 界面配置
   - 添加配置备份和恢复功能

2. **安全性**:
   - 考虑加密存储敏感信息（如密码、API Key）
   - 添加配置访问密码保护

3. **用户体验**:
   - 添加配置向导模式
   - 支持配置模板
   - 改进错误提示信息

## 测试建议

1. **基本功能测试**:
   - 测试各项配置的设置和保存
   - 测试配置加载和验证
   - 测试配置清除功能

2. **边界测试**:
   - 测试超长字符串输入
   - 测试无效的 MAC 地址格式
   - 测试 EEPROM 损坏情况

3. **集成测试**:
   - 测试配置后的 WiFi 连接
   - 测试配置后的天气数据获取
   - 测试深度睡眠唤醒后配置保持

## 文档

详细使用说明请参考：[`lib/SerialConfig/README.md`](lib/SerialConfig/README.md)

## 总结

成功实现了完整的串口配置功能，代码结构清晰，功能完善，编译通过。该功能使设备配置更加灵活，无需修改代码即可更改设备参数。
