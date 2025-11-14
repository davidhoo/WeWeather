#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <Arduino.h>

class BatteryMonitor {
private:
    uint8_t adcPin;
    float voltageMultiplier;
    float minVoltage;
    float maxVoltage;
    
    // 内部方法
    float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);
    
public:
    // 构造函数
    BatteryMonitor(uint8_t pin = A0);
    
    // 初始化方法
    void begin();
    
    // 获取原始 ADC 值
    int getRawADC();
    
    // 获取电池电压（伏特）
    float getBatteryVoltage();
    
    // 获取电池电量百分比
    float getBatteryPercentage();
    
    // 设置电压范围（可选，用于校准）
    void setVoltageRange(float minV, float maxV);
};

#endif