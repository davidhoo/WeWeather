# BatteryMonitor Library

用于监控 3.7V 锂聚合物电池的 Arduino 库。

## 功能特性

- 通过 ADC 直接测量电池电压
- 获取电池电量百分比
- 获取原始 ADC 测量值
- 支持电压范围校准

## 硬件连接

- 电池正极连接到 ESP8266 的 A0 引脚
- 电池负极连接到 GND

## 使用方法

```cpp
#include "BatteryMonitor.h"

BatteryMonitor battery;

void setup() {
    Serial.begin(74880);
    battery.begin();
}

void loop() {
    // 获取原始 ADC 值
    int rawADC = battery.getRawADC();
    
    // 获取电池电压
    float voltage = battery.getBatteryVoltage();
    
    // 获取电池电量百分比
    float percentage = battery.getBatteryPercentage();
    
    Serial.printf("ADC: %d, 电压: %.2fV, 电量: %.1f%%\n", 
                  rawADC, voltage, percentage);
    
    delay(5000);
}
```

## API 参考

### 构造函数
- `BatteryMonitor(uint8_t pin = A0)` - 创建电池监控实例

### 方法
- `void begin()` - 初始化电池监控
- `int getRawADC()` - 获取原始 ADC 值 (0-1023)
- `float getBatteryVoltage()` - 获取电池电压（伏特）
- `float getBatteryPercentage()` - 获取电池电量百分比 (0-100%)
- `void setVoltageRange(float minV, float maxV)` - 设置电压范围用于校准

## 技术规格

- 支持的电池：3.7V 锂聚合物电池
- 电压范围：3.0V - 4.2V
- ADC 分辨率：10 位 (0-1023)
- 参考电压：3.3V