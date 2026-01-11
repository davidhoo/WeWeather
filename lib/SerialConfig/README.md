# 串口配置功能使用说明

## 功能概述

本项目新增了串口配置模式，允许通过串口命令配置以下信息并保存到 EEPROM：

1. WiFi SSID
2. WiFi 密码
3. 自定义 MAC 地址
4. 高德地图 API Key (AMAP_API_KEY)
5. 城市代码 (cityCode)

配置信息会优先使用 EEPROM 中保存的设置，如果没有配置则使用代码中的默认值。

## 进入配置模式

设备启动后，会有 **5 秒钟**的等待时间。在这 5 秒内，通过串口发送 `config` 命令即可进入配置模式。

```
等待配置命令... (5秒内发送 'config' 进入配置模式)
```

发送 `config` 后，系统会进入配置模式并显示帮助信息。

## 可用命令

进入配置模式后，可以使用以下命令：

### 设置配置项

```bash
set ssid <SSID>              # 设置 WiFi SSID
set password <password>        # 设置 WiFi 密码
set mac <MAC>                # 设置 MAC 地址 (格式: AA:BB:CC:DD:EE:FF)
set apikey <KEY>             # 设置高德地图 API Key
set citycode <CODE>          # 设置城市代码
```

### 管理配置

```bash
save                         # 保存配置到 EEPROM
show                         # 显示当前配置
clear                        # 清除配置（需要输入 yes 确认）
help                         # 显示帮助信息
exit                         # 退出配置模式
```

## 使用示例

### 完整配置流程

1. 设备启动后，在 5 秒内发送 `config`
2. 进入配置模式后，依次设置各项配置：

```bash
> set ssid MyWiFi
✓ SSID 已设置

> set password Mypassword123
✓ 密码已设置

> set mac AA:BB:CC:DD:EE:FF
✓ MAC 地址已设置

> set apikey your_amap_api_key_here
✓ 高德地图 API Key 已设置

> set citycode 110108
✓ 城市代码已设置

> show
=== 设备配置 ===
已配置: 否
SSID: MyWiFi
密码: ***
MAC 地址: AA:BB:CC:DD:EE:FF
高德 API Key: ***
城市代码: 110108
校验和: 0x1234
================

> save
✓ 配置已成功保存到 EEPROM

> exit
退出配置模式
```

3. 配置保存后，设备会继续正常启动流程，使用刚才配置的参数

### 查看当前配置

```bash
> show
=== 设备配置 ===
已配置: 是
SSID: MyWiFi
密码: ***
MAC 地址: AA:BB:CC:DD:EE:FF
高德 API Key: ***
城市代码: 110108
校验和: 0x1234
================
```

### 清除配置

```bash
> clear
警告: 即将清除所有配置!
输入 'yes' 确认清除: 
> yes
✓ 配置已清除
```

## 配置优先级

系统启动时会按以下优先级加载配置：

1. **EEPROM 配置**：如果 EEPROM 中有有效配置，优先使用
2. **默认配置**：如果 EEPROM 中没有配置或配置无效，使用代码中的默认值

## 技术细节

### EEPROM 存储结构

配置数据存储在 EEPROM 的起始地址（地址 0），包含以下字段：

```c
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

### 数据校验

- 配置数据使用校验和验证完整性
- 启动时会自动验证 EEPROM 中的配置
- 如果校验失败，会使用默认配置

### 配置超时

- 进入配置模式后，如果 60 秒内没有操作，会自动退出配置模式
- 可以随时使用 `exit` 命令手动退出

## 注意事项

1. **MAC 地址格式**：必须使用标准格式 `AA:BB:CC:DD:EE:FF`（大小写均可）
2. **字符串长度限制**：
   - SSID: 最多 31 个字符
   - 密码: 最多 63 个字符
   - MAC 地址: 固定 17 个字符
   - API Key: 最多 63 个字符
   - 城市代码: 最多 15 个字符
3. **保存配置**：设置完成后必须执行 `save` 命令才会保存到 EEPROM
4. **清除确认**：清除配置需要输入 `yes` 确认，防止误操作

## 串口设置

- 波特率：74880
- 数据位：8
- 停止位：1
- 校验位：无

## 相关库

- **ConfigManager**：管理 EEPROM 配置的读写
- **SerialConfig**：处理串口命令和用户交互
