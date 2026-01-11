# WebConfig 库

## 概述

WebConfig 库为 WeWeather 项目提供基于 Web 的配置界面。通过 AP 模式和 Web 服务器，用户可以通过浏览器配置设备参数。

## 功能特性

- **AP 模式**: 设备创建一个 WiFi 热点（默认 SSID: weweather）
- **Web 界面**: 提供友好的 HTML 配置页面
- **配置项管理**: 支持与串口配置相同的所有配置项
  - WiFi SSID
  - WiFi 密码
  - MAC 地址
  - 高德地图 API Key
  - 城市代码
- **屏幕显示**: 在墨水屏上显示 AP 信息和访问地址
- **配置验证**: 自动验证和保存配置到 EEPROM

## 使用方法

### 进入 Web 配置模式

1. **硬件触发**: 在设备启动时，将 RXD 引脚（GPIO-3）拉低
2. 设备会自动：
   - 禁用 RTC 定时器和中断
   - 启动 AP 模式（SSID: weweather）
   - 在屏幕上显示连接信息
   - 启动 Web 服务器

### 配置步骤

1. 使用手机或电脑连接到 WiFi: `weweather`
2. 打开浏览器访问: `http://192.168.4.1`
3. 填写配置信息：
   - WiFi SSID（必填）
   - WiFi 密码（可选，留空保持不变）
   - MAC 地址（可选）
   - 高德地图 API Key（可选）
   - 城市代码（可选）
4. 点击"保存配置"按钮
5. 配置成功后，设备会自动重启并应用新配置

### 超时设置

- 默认超时时间: 5 分钟（300000 毫秒）
- 超时后自动退出配置模式，继续正常启动

## API 参考

### 构造函数

```cpp
WebConfig(ConfigManager* configManager);
```

### 主要方法

#### begin()
```cpp
void begin(const char* apSSID = "weweather", const char* apPassword = "");
```
初始化 Web 配置模式，启动 AP 和 Web 服务器。

**参数:**
- `apSSID`: AP 的 SSID（默认: "weweather"）
- `apPassword`: AP 的密码（默认: 无密码）

#### enterConfigMode()
```cpp
bool enterConfigMode(unsigned long timeout = 300000);
```
进入配置模式，阻塞等待用户配置或超时。

**参数:**
- `timeout`: 超时时间（毫秒），默认 5 分钟

**返回值:**
- `true`: 配置成功
- `false`: 配置超时

#### getAPIP()
```cpp
String getAPIP();
```
获取 AP 的 IP 地址。

#### getAPSSID()
```cpp
String getAPSSID();
```
获取 AP 的 SSID。

#### stop()
```cpp
void stop();
```
停止 Web 服务器和 AP 模式。

## 示例代码

```cpp
#include "../lib/WebConfig/WebConfig.h"
#include "../lib/ConfigManager/ConfigManager.h"

ConfigManager configManager;
WebConfig webConfig(&configManager);

void setup() {
  Serial.begin(74880);
  
  // 初始化 ConfigManager
  configManager.begin();
  
  // 检查是否需要进入 Web 配置模式
  pinMode(RXD_PIN, INPUT_PULLUP);
  if (digitalRead(RXD_PIN) == LOW) {
    // 启动 Web 配置
    webConfig.begin("weweather", "");
    
    // 显示信息到屏幕
    String apSSID = webConfig.getAPSSID();
    String apIP = webConfig.getAPIP();
    epd.showWebConfigInfo(apSSID, apIP);
    
    // 进入配置模式
    if (webConfig.enterConfigMode(300000)) {
      Serial.println("配置成功，重启...");
      ESP.restart();
    }
  }
}
```

## 注意事项

1. **RTC 定时器**: 进入配置模式前，必须禁用 RTC 定时器和中断，避免配置过程中被唤醒
2. **内存使用**: Web 服务器会占用一定内存，配置完成后应及时调用 `stop()` 释放资源
3. **安全性**: 默认 AP 无密码，如需安全连接，可在 `begin()` 时设置密码
4. **配置验证**: 所有配置会自动验证并保存到 EEPROM，保存失败会返回错误页面

## 与串口配置的对比

| 特性 | Web 配置 | 串口配置 |
|------|---------|---------|
| 触发方式 | RXD 引脚拉低 | 启动后发送 "config" |
| 用户界面 | Web 浏览器 | 串口终端 |
| 配置项 | 完全相同 | 完全相同 |
| 超时时间 | 5 分钟 | 1 分钟 |
| 屏幕显示 | 显示连接信息 | 无 |
| 便利性 | 更友好，支持移动设备 | 需要串口连接 |

## 技术细节

- **Web 服务器**: 基于 ESP8266WebServer
- **AP 模式**: 使用 ESP8266 的 SoftAP 功能
- **默认 IP**: 192.168.4.1
- **端口**: 80 (HTTP)
- **HTML 编码**: UTF-8，支持中文
- **响应式设计**: 适配移动设备和桌面浏览器
