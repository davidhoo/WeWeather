#include "BatteryMonitor.h"

// 构造函数
BatteryMonitor::BatteryMonitor(uint8_t pin) {
    adcPin = pin;
    // ESP8266 ADC 参考电压为 3.3V，ADC 分辨率为 10 位 (0-1023)
    voltageMultiplier = 3.3 / 1023.0;
    // 3.7V 锂电池的典型电压范围
    minVoltage = 3.0;  // 电池最低电压
    maxVoltage = 4.2;  // 电池最高电压（充满电）
}

// 初始化方法
void BatteryMonitor::begin() {
    // ESP8266 的 A0 引脚不需要特殊初始化
    // ADC 默认已经配置好
}

// 获取原始 ADC 值
int BatteryMonitor::getRawADC() {
    return analogRead(adcPin);
}

// 获取电池电压
float BatteryMonitor::getBatteryVoltage() {
    int rawValue = getRawADC();
    float voltage = rawValue * voltageMultiplier;
    return voltage;
}

// 获取电池电量百分比
float BatteryMonitor::getBatteryPercentage() {
    float voltage = getBatteryVoltage();
    
    // 限制电压范围
    if (voltage < minVoltage) {
        voltage = minVoltage;
    }
    if (voltage > maxVoltage) {
        voltage = maxVoltage;
    }
    
    // 将电压映射到 0-100% 范围
    float percentage = mapFloat(voltage, minVoltage, maxVoltage, 0.0, 100.0);
    
    return percentage;
}

// 设置电压范围（用于校准）
void BatteryMonitor::setVoltageRange(float minV, float maxV) {
    minVoltage = minV;
    maxVoltage = maxV;
}

// 内部辅助方法：浮点数映射
float BatteryMonitor::mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}