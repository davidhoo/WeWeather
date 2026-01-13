# WebConfigManager 库

Web 配置管理库，提供通过 Web 界面进行设备配置的功能，支持基于 ESP8266WebServer 的配置页面。

## 功能特性

- Web 服务器初始化和路由处理
- 配置参数的 Web 界面显示、设置和保存
- 配置请求的处理
- 友好的 Web 配置界面
- 配置验证和错误处理
- 支持退出配置模式并重启系统

## 使用方法

### 基本初始化

```cpp
#include "WebConfigManager.h"
#include "ConfigManager.h"

// 创建配置管理器
ConfigManager<ConfigData> configManager(0, 512);

// 创建 Web 配置管理器
WebConfigManager webConfig(&configManager);

void setup() {
    Serial.begin(115200);
    
    // 启动 Web 配置服务
    if (webConfig.startConfigService()) {
        Serial.println("Web 配置服务启动成功");
        Serial.println("请连接 WiFi 热点并访问配置页面");
    } else {
        Serial.println("Web 配置服务启动失败");
    }
}

void loop() {
    // 处理 Web 请求
    webConfig.handleClient();
}
```

### 手动启动和停止 Web 服务器

```cpp
void manageWebServer() {
    // 启动 Web 服务器（默认端口 80）
    if (webConfig.startWebServer(80)) {
        Serial.println("Web 服务器启动成功");
        
        // 设置配置模式
        webConfig.setConfigMode(true);
        
        // 处理客户端请求
        while (webConfig.isInConfigMode()) {
            webConfig.handleClient();
            delay(10);
        }
    }
    
    // 停止 Web 服务器
    webConfig.stopWebServer();
}
```

### 配置模式管理

```cpp
void configModeExample() {
    // 检查是否处于配置模式
    if (webConfig.isInConfigMode()) {
        Serial.println("当前处于配置模式");
        
        // 处理 Web 请求
        webConfig.handleClient();
        
        // 用户完成配置后退出
        // webConfig.exitConfigMode();
    } else {
        Serial.println("当前处于正常运行模式");
    }
}
```

## Web 界面

### 配置页面

访问 `http://<设备IP>/config` 进入配置页面，包含以下配置项：

- WiFi SSID
- WiFi 密码
- 高德地图 API 密钥
- 城市代码
- 其他系统配置

### 页面路由

- `/` - 根页面，显示设备信息和配置链接
- `/config` - 配置页面，显示和修改配置参数
- `/save` - 保存配置，处理配置表单提交
- `/exit` - 退出配置模式并重启系统
- `/*` - 404 页面，处理未找到的请求

## API 参考

### 构造函数
- `WebConfigManager(ConfigManager<ConfigData>* configMgr)` - 创建 Web 配置管理器实例

### Web 服务器管理
- `bool startWebServer(int port = 80)` - 启动 Web 服务器
- `void stopWebServer()` - 停止 Web 服务器
- `bool startConfigService()` - 启动 Web 配置服务
- `void handleClient()` - 处理 Web 请求

### 配置模式管理
- `void exitConfigMode()` - 退出配置模式并重启系统
- `bool isInConfigMode() const` - 检查是否处于配置模式
- `void setConfigMode(bool enabled)` - 设置配置模式状态

## 使用场景

### 1. 设备首次配置

```cpp
void firstTimeSetup() {
    // 检查是否有有效配置
    if (!configManager.hasValidEEPROMConfig()) {
        Serial.println("首次启动，进入 Web 配置模式");
        
        // 启动 Web 配置服务
        webConfig.startConfigService();
        
        // 等待用户配置
        while (webConfig.isInConfigMode()) {
            webConfig.handleClient();
            delay(100);
        }
        
        Serial.println("配置完成，重启系统");
    }
}
```

### 2. 运行时配置更新

```cpp
void runtimeConfigUpdate() {
    // 检查是否需要进入配置模式
    if (shouldEnterConfigMode()) {
        Serial.println("进入 Web 配置模式");
        
        // 启动 Web 服务器
        webConfig.startWebServer(8080);
        webConfig.setConfigMode(true);
        
        // 处理配置请求
        while (webConfig.isInConfigMode()) {
            webConfig.handleClient();
            delay(10);
        }
        
        // 配置完成后自动重启
        webConfig.exitConfigMode();
    }
}
```

### 3. 多配置模式支持

```cpp
void multiConfigMode() {
    // 同时支持串口和 Web 配置
    SerialConfigManager serialConfig(&configManager);
    WebConfigManager webConfig(&configManager);
    
    // 启动两种配置服务
    serialConfig.startConfigService();
    webConfig.startConfigService();
    
    // 处理两种输入
    while (true) {
        serialConfig.processInput();
        webConfig.handleClient();
        delay(10);
    }
}
```

## 错误处理

库提供了完善的错误处理机制：

1. **Web 服务器启动失败**：返回 false，提供错误信息
2. **配置验证失败**：显示错误页面，提示用户重新输入
3. **网络连接问题**：自动重试，提供友好的错误提示
4. **内存不足**：优雅降级，避免系统崩溃

## 安全特性

1. **输入验证**：所有用户输入都进行验证和清理
2. **缓冲区保护**：防止缓冲区溢出攻击
3. **配置保护**：敏感信息（如密码）适当处理
4. **访问控制**：限制配置页面的访问权限

## 性能优化

1. **轻量级 Web 服务器**：基于 ESP8266WebServer，资源占用少
2. **静态页面生成**：减少动态内存分配
3. **连接池管理**：优化并发连接处理
4. **缓存机制**：减少重复配置读取

## 注意事项

1. **网络环境**：确保设备在可访问的网络环境中
2. **端口冲突**：避免与其他服务使用相同端口
3. **配置保存**：配置更改后需要重启系统才能生效
4. **安全性**：在生产环境中考虑添加身份验证
5. **内存使用**：Web 服务器会占用一定的内存资源

## 依赖库

- ESP8266WiFi：WiFi 连接管理
- ESP8266WebServer：Web 服务器功能
- ConfigManager：配置数据管理
- Arduino：基础 Arduino 框架

## 配置页面示例

### HTML 结构

```html
<!DOCTYPE html>
<html>
<head>
    <title>WeWeather 配置</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; }
        .container { max-width: 600px; margin: 0 auto; }
        .form-group { margin-bottom: 15px; }
        label { display: block; margin-bottom: 5px; font-weight: bold; }
        input { width: 100%; padding: 8px; box-sizing: border-box; }
        button { padding: 10px 20px; background-color: #4CAF50; color: white; border: none; cursor: pointer; }
        button:hover { background-color: #45a049; }
        .error { color: red; margin-top: 5px; }
        .success { color: green; margin-top: 5px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>WeWeather 配置</h1>
        <form action="/save" method="post">
            <div class="form-group">
                <label for="wifi_ssid">WiFi SSID:</label>
                <input type="text" id="wifi_ssid" name="wifi_ssid" value="<%= wifi_ssid %>">
            </div>
            <div class="form-group">
                <label for="wifi_password">WiFi 密码:</label>
                <input type="password" id="wifi_password" name="wifi_password" value="<%= wifi_password %>">
            </div>
            <div class="form-group">
                <label for="amap_api_key">高德地图 API 密钥:</label>
                <input type="text" id="amap_api_key" name="amap_api_key" value="<%= amap_api_key %>">
            </div>
            <div class="form-group">
                <label for="city_code">城市代码:</label>
                <input type="text" id="city_code" name="city_code" value="<%= city_code %>">
            </div>
            <button type="submit">保存配置</button>
            <a href="/exit">退出配置模式</a>
        </form>
    </div>
</body>
</html>
```

## 示例项目

### 完整配置系统

```cpp
#include "WebConfigManager.h"
#include "SerialConfigManager.h"
#include "ConfigManager.h"

ConfigManager<ConfigData> configManager(0, 512);
WebConfigManager webConfig(&configManager);
SerialConfigManager serialConfig(&configManager);

void setup() {
    Serial.begin(115200);
    
    // 初始化配置管理器
    configManager.begin();
    
    // 检查是否需要配置
    if (!configManager.hasValidEEPROMConfig()) {
        Serial.println("设备未配置，进入配置模式");
        
        // 启动两种配置服务
        serialConfig.startConfigService();
        webConfig.startConfigService();
        
        // 等待配置完成
        while (true) {
            serialConfig.processInput();
            webConfig.handleClient();
            
            // 检查配置是否完成
            if (configManager.hasValidEEPROMConfig()) {
                Serial.println("配置完成，重启系统");
                ESP.restart();
            }
            
            delay(10);
        }
    }
    
    Serial.println("设备已配置，启动正常服务");
    startNormalService();
}

void loop() {
    // 正常服务循环
    runNormalService();
}
```

## 故障排除

### 常见问题

1. **无法访问配置页面**
   - 检查设备 IP 地址
   - 确认网络连接正常
   - 检查防火墙设置

2. **配置保存失败**
   - 检查 EEPROM 空间
   - 验证配置数据格式
   - 检查网络连接

3. **Web 服务器崩溃**
   - 检查内存使用情况
   - 减少并发连接数
   - 优化页面内容

### 调试信息

启用调试模式以获取详细日志：

```cpp
#define DEBUG_WEB_CONFIG 1
```

调试信息包括：
- Web 请求处理状态
- 配置保存结果
- 错误信息和堆栈跟踪
- 内存使用情况