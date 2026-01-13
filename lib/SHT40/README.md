# SHT40 库

SHT40 数字温湿度传感器的 Arduino 驱动库，支持 I2C 通信协议，提供高精度的温度和湿度测量。

## 功能特性

- 高精度温度测量（±0.2°C）
- 高精度湿度测量（±1.5%RH）
- 多种测量精度模式可选
- I2C 数字接口
- CRC 数据校验
- 软件复位功能
- 序列号读取
- 低功耗设计

## 硬件连接

- VCC → 3.3V/5V
- GND → GND
- SDA → 指定的 SDA 引脚
- SCL → 指定的 SCL 引脚
- I2C 地址：0x44（默认）

## 使用方法

### 基本初始化和测量

```cpp
#include "SHT40.h"

// 创建 SHT40 实例，指定 SDA 和 SCL 引脚
SHT40 sht40(21, 22);  // ESP32: SDA=21, SCL=22

void setup() {
    Serial.begin(115200);
    
    // 初始化传感器
    if (!sht40.begin()) {
        Serial.println("SHT40 初始化失败");
        return;
    }
    
    Serial.println("SHT40 初始化成功");
}

void loop() {
    float temperature, humidity;
    
    // 读取温湿度数据
    if (sht40.readTemperatureHumidity(temperature, humidity)) {
        Serial.printf("温度: %.2f°C, 湿度: %.2f%%\n", temperature, humidity);
    } else {
        Serial.println("读取失败");
    }
    
    delay(2000);
}
```

### 单独读取温度或湿度

```cpp
void readSeparateValues() {
    // 只读取温度
    float temperature = sht40.readTemperature();
    if (temperature != NAN) {
        Serial.printf("温度: %.2f°C\n", temperature);
    }
    
    // 只读取湿度
    float humidity = sht40.readHumidity();
    if (humidity != NAN) {
        Serial.printf("湿度: %.2f%%\n", humidity);
    }
}
```

### 传感器信息读取

```cpp
void readSensorInfo() {
    // 读取序列号
    uint32_t serialNumber;
    if (sht40.readSerialNumber(serialNumber)) {
        Serial.printf("传感器序列号: %08X\n", serialNumber);
    }
    
    // 软件复位
    if (sht40.softReset()) {
        Serial.println("传感器复位成功");
    }
}
```

## API 参考

### 构造函数
- `SHT40(uint8_t sda_pin, uint8_t scl_pin, uint8_t addr = SHT40_ADDR)` - 创建 SHT40 实例

### 基本方法
- `bool begin()` - 初始化传感器
- `bool readTemperatureHumidity(float &temperature, float &humidity)` - 读取温湿度数据
- `float readTemperature()` - 读取温度（摄氏度）
- `float readHumidity()` - 读取湿度（%RH）

### 高级方法
- `bool softReset()` - 软件复位传感器
- `bool readSerialNumber(uint32_t &serialNumber)` - 读取传感器序列号

## 测量模式

SHT40 支持多种测量精度模式：

### 高精度模式（推荐）
- 命令：`0xFD`（单字节）或 `0x2C06`（双字节）
- 温度精度：±0.2°C
- 湿度精度：±1.5%RH
- 测量时间：约 8.2ms

### 中精度模式
- 命令：`0xF6`（单字节）或 `0x2C0D`（双字节）
- 温度精度：±0.4°C
- 湿度精度：±2.0%RH
- 测量时间：约 4.5ms

### 低精度模式
- 命令：`0xE0`（单字节）或 `0x2C10`（双字节）
- 温度精度：±0.5°C
- 湿度精度：±3.0%RH
- 测量时间：约 1.7ms

## 常量定义

### 测量命令
- `MEAS_HIGHREP_STRETCH` - 高精度测量（时钟拉伸）
- `MEAS_MEDREP_STRETCH` - 中精度测量（时钟拉伸）
- `MEAS_LOWREP_STRETCH` - 低精度测量（时钟拉伸）
- `MEAS_HIGHREP` - 高精度测量
- `MEAS_MEDREP` - 中精度测量
- `MEAS_LOWREP` - 低精度测量

### 其他命令
- `READSERIAL` - 读取序列号
- `SOFTRESET` - 软件复位

## 数据格式

### 温度数据
- 范围：-40°C 到 +125°C
- 分辨率：0.01°C
- 输出格式：浮点数（摄氏度）

### 湿度数据
- 范围：0%RH 到 100%RH
- 分辨率：0.01%RH
- 输出格式：浮点数（相对湿度百分比）

## 错误处理

库提供了完善的错误处理机制：

1. **通信错误**：I2C 通信失败时返回 false 或 NAN
2. **CRC 校验**：自动验证数据完整性
3. **超时处理**：防止长时间等待传感器响应
4. **初始化检查**：启动时验证传感器连接

## 性能优化建议

1. **测量频率**：根据应用需求选择合适的测量间隔
2. **精度模式**：在精度和速度之间权衡
3. **功耗管理**：在电池供电时考虑低精度模式
4. **数据缓存**：避免频繁读取相同数据

## 技术规格

- 工作电压：2.4V - 5.5V
- 工作温度：-40°C 到 +125°C
- 湿度测量范围：0%RH 到 100%RH
- I2C 接口：标准模式（100kHz）和快速模式（400kHz）
- 平均功耗：0.4μA（待机），1.5mA（测量中）
- 响应时间：< 8s（63% 步进响应）

## 注意事项

1. **引脚配置**：确保正确连接 SDA 和 SCL 引脚
2. **电源稳定**：提供稳定的电源供应
3. **环境条件**：避免在极端环境下长期使用
4. **校准**：传感器出厂已校准，无需额外校准
5. **防冷凝**：避免在冷凝环境下使用

## 依赖库

- Wire：I2C 通信库
- Arduino：基础 Arduino 框架

## 示例项目

### 天气监测站

```cpp
#include "SHT40.h"

SHT40 sht40(21, 22);

void setup() {
    Serial.begin(115200);
    
    if (!sht40.begin()) {
        Serial.println("传感器初始化失败");
        while (1);
    }
    
    Serial.println("天气监测站启动");
}

void loop() {
    float temp, hum;
    
    if (sht40.readTemperatureHumidity(temp, hum)) {
        Serial.printf("当前环境 - 温度: %.1f°C, 湿度: %.1f%%\n", temp, hum);
        
        // 简单的舒适度判断
        if (temp >= 20 && temp <= 26 && hum >= 40 && hum <= 60) {
            Serial.println("环境舒适");
        } else {
            Serial.println("环境需要调节");
        }
    }
    
    delay(5000);  // 每5秒更新一次
}