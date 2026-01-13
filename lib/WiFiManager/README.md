# WiFiManager 库

WiFi 连接管理库，提供简化的 WiFi 连接、配置和管理功能，支持自动重连、网络扫描和 MAC 地址自定义。

## 功能特性

- 简化的 WiFi 连接接口
- 自动重连机制
- 网络扫描功能
- MAC 地址自定义
- 连接状态监控
- 信号强度检测
- 灵活的配置管理

## 使用方法

### 基本初始化

```cpp
#include "WiFiManager.h"

// 创建 WiFi 管理器实例
WiFiManager wifiManager;

void setup() {
    Serial.begin(115200);
    
    // 使用默认配置初始化
    wifiManager.begin();
    
    // 连接到 WiFi
    if (wifiManager.connect()) {
        Serial.println("WiFi 连接成功");
        Serial.printf("IP 地址: %s\n", wifiManager.getLocalIP().c_str());
        Serial.printf("信号强度: %d dBm\n", wifiManager.getRSSI());
    } else {
        Serial.println("WiFi 连接失败");
    }
}
```

### 自定义配置初始化

```cpp
void setupWithCustomConfig() {
    // 创建自定义配置
    WiFiConfig customConfig = {
        .ssid = "MyWiFiNetwork",
        .password = "Mypassword123",
        .timeout = 30000,  // 30秒超时
        .autoReconnect = true,
        .maxRetries = 5,
        .macAddress = "AA:BB:CC:DD:EE:FF",
        .useMacAddress = false
    };
    
    // 使用自定义配置初始化
    wifiManager.begin(customConfig);
    
    // 自动连接
    if (wifiManager.autoConnect()) {
        Serial.println("自动连接成功");
    }
}
```

### 动态配置更新

```cpp
void updateWiFiConfig() {
    // 设置新的 WiFi 凭据
    wifiManager.setCredentials("NewWiFiSSID", "Newpassword");
    
    // 设置连接超时时间
    wifiManager.setTimeout(60000);  // 60秒
    
    // 启用自动重连
    wifiManager.setAutoReconnect(true);
    
    // 设置最大重试次数
    wifiManager.setMaxRetries(10);
    
    // 设置自定义 MAC 地址
    wifiManager.setMacAddress("12:34:56:78:9A:BC");
    wifiManager.enableMacAddress(true);
    
    // 重新连接
    wifiManager.connect();
}
```

### 网络扫描和选择

```cpp
void scanAndConnect() {
    Serial.println("扫描可用网络...");
    
    int networkCount = wifiManager.scanNetworks();
    Serial.printf("发现 %d 个网络\n", networkCount);
    
    // 显示网络信息
    for (int i = 0; i < networkCount; i++) {
        String ssid = wifiManager.getScannedSSID(i);
        int rssi = wifiManager.getScannedRSSI(i);
        bool secure = wifiManager.isScannedNetworkSecure(i);
        
        Serial.printf("%d: %s (%d dBm) %s\n", 
                      i + 1, ssid.c_str(), rssi, 
                      secure ? "[安全]" : "[开放]");
    }
    
    // 尝试连接到指定网络
    if (wifiManager.scanAndConnect()) {
        Serial.println("扫描并连接成功");
    }
}
```

## API 参考

### 构造函数
- `WiFiManager()` - 创建 WiFi 管理器实例

### 初始化方法
- `void begin()` - 使用默认配置初始化
- `void begin(const WiFiConfig& config)` - 使用自定义配置初始化

### 配置管理
- `void setDefaultConfig()` - 设置默认配置
- `void setCredentials(const char* ssid, const char* password)` - 设置 WiFi 凭据
- `void setConfig(const WiFiConfig& config)` - 设置完整配置
- `WiFiConfig getConfig() const` - 获取当前配置

### 连接管理
- `bool connect(unsigned long timeout = 0)` - 连接到 WiFi 网络
- `bool scanAndConnect(unsigned long timeout = 0)` - 扫描并连接到指定网络
- `bool autoConnect()` - 自动连接
- `bool isConnected()` - 检查连接状态
- `void disconnect()` - 断开连接

### 信息获取
- `String getLocalIP()` - 获取本地 IP 地址
- `int getRSSI()` - 获取信号强度
- `String getMacAddress()` - 获取 MAC 地址
- `String getStatusString()` - 获取状态描述

### 网络扫描
- `int scanNetworks()` - 扫描可用网络
- `String getScannedSSID(int index)` - 获取扫描到的网络 SSID
- `int getScannedRSSI(int index)` - 获取扫描到的网络信号强度
- `bool isScannedNetworkSecure(int index)` - 检查网络是否安全

### 高级设置
- `void setTimeout(unsigned long timeout)` - 设置连接超时时间
- `void setAutoReconnect(bool enable)` - 设置自动重连
- `void setMaxRetries(int retries)` - 设置最大重试次数
- `void setMacAddress(const char* macAddress)` - 设置 MAC 地址
- `void enableMacAddress(bool enable)` - 启用/禁用自定义 MAC 地址

### 调试工具
- `void printConfig()` - 打印当前配置信息

## 数据结构

### WiFiConfig 结构体

```cpp
struct WiFiConfig {
    char ssid[32];           // WiFi 网络名称
    char password[64];    // WiFi 密码
    unsigned long timeout;   // 连接超时时间（毫秒）
    bool autoReconnect;      // 是否自动重连
    int maxRetries;          // 最大重试次数
    char macAddress[18];     // MAC 地址字符串
    bool useMacAddress;      // 是否使用自定义 MAC 地址
};
```

## 使用场景

### 1. 自动重连机制

```cpp
void maintainWiFiConnection() {
    // 检查连接状态
    if (!wifiManager.isConnected()) {
        Serial.println("WiFi 连接断开，尝试重连...");
        
        // 自动重连
        if (wifiManager.autoConnect()) {
            Serial.println("重连成功");
        } else {
            Serial.println("重连失败");
        }
    }
}
```

### 2. 信号强度监控

```cpp
void monitorSignalStrength() {
    int rssi = wifiManager.getRSSI();
    
    if (rssi > -50) {
        Serial.println("信号强度：优秀");
    } else if (rssi > -70) {
        Serial.println("信号强度：良好");
    } else if (rssi > -80) {
        Serial.println("信号强度：一般");
    } else {
        Serial.println("信号强度：差");
    }
}
```

### 3. 多网络切换

```cpp
void multiNetworkSupport() {
    // 尝试连接主网络
    wifiManager.setCredentials("MainNetwork", "Mainpassword");
    if (!wifiManager.connect()) {
        Serial.println("主网络连接失败，尝试备用网络");
        
        // 切换到备用网络
        wifiManager.setCredentials("BackupNetwork", "Backuppassword");
        wifiManager.connect();
    }
}
```

## 错误处理

库提供了完善的错误处理机制：

1. **连接超时**：自动重试，达到最大重试次数后返回失败
2. **网络不可用**：提供详细的错误信息和状态描述
3. **配置错误**：验证配置参数，防止无效配置
4. **内存不足**：优雅降级，避免系统崩溃

## 性能优化

1. **智能重连**：根据信号强度和历史连接情况优化重连策略
2. **连接缓存**：缓存连接信息，减少重复连接开销
3. **异步扫描**：非阻塞网络扫描，不影响主程序运行
4. **资源管理**：合理管理 WiFi 资源，减少功耗

## 注意事项

1. **网络环境**：确保目标网络在设备范围内
2. **安全协议**：支持 WPA/WPA2 安全协议
3. **MAC 地址**：自定义 MAC 地址需要符合格式要求
4. **功耗管理**：长时间连接时考虑功耗优化
5. **并发连接**：避免同时进行多个连接操作

## 依赖库

- ESP8266WiFi：ESP8266 WiFi 功能库
- Arduino：基础 Arduino 框架

## 配置示例

### 默认配置

```cpp
WiFiConfig defaultConfig = {
    .ssid = "",              // 空，需要用户设置
    .password = "",       // 空，需要用户设置
    .timeout = 30000,        // 30秒超时
    .autoReconnect = true,   // 启用自动重连
    .maxRetries = 5,         // 最大重试次数
    .macAddress = "",        // 空，使用设备默认 MAC
    .useMacAddress = false   // 不使用自定义 MAC
};
```

### 生产环境配置

```cpp
WiFiConfig productionConfig = {
    .ssid = "ProductionWiFi",
    .password = "Securepassword123",
    .timeout = 60000,        // 60秒超时
    .autoReconnect = true,   // 启用自动重连
    .maxRetries = 10,        // 最大重试次数
    .macAddress = "00:11:22:33:44:55",
    .useMacAddress = true    // 使用自定义 MAC
};
```

## 故障排除

### 常见问题

1. **连接失败**
   - 检查 SSID 和密码是否正确
   - 确认网络在设备范围内
   - 检查路由器设置

2. **信号强度差**
   - 调整设备位置
   - 检查天线连接
   - 考虑使用信号放大器

3. **自动重连失败**
   - 检查网络可用性
   - 调整重试参数
   - 检查设备状态

### 调试信息

启用调试模式以获取详细日志：

```cpp
#define DEBUG_WIFI_MANAGER 1
```

调试信息包括：
- 连接状态变化
- 网络扫描结果
- 信号强度变化
- 错误信息和堆栈跟踪