# BM8563 库

BM8563 实时时钟（RTC）芯片的 Arduino 驱动库，支持 I2C 通信协议。

## 功能特性

- 完整的实时时钟功能（年、月、日、星期、时、分、秒）
- 闹钟功能，支持中断唤醒
- 定时器功能，多种频率可选
- CLKOUT 时钟输出功能
- 电源失效检测
- 低功耗设计

## 硬件连接

- VCC → 3.3V/5V
- GND → GND
- SDA → 指定的 SDA 引脚
- SCL → 指定的 SCL 引脚
- I2C 地址：0x51

## 使用方法

### 基本初始化和时间设置

```cpp
#include "BM8563.h"

// 创建 BM8563 实例，指定 SDA 和 SCL 引脚
BM8563 rtc(21, 22);  // ESP32: SDA=21, SCL=22

void setup() {
    Serial.begin(115200);
    
    // 初始化 RTC
    if (!rtc.begin()) {
        Serial.println("BM8563 初始化失败");
        return;
    }
    
    // 设置时间
    BM8563_Time time = {
        .seconds = 0,
        .minutes = 30,
        .hours = 14,
        .days = 15,
        .weekdays = 1,  // 星期一 (1-7)
        .months = 1,
        .years = 24     // 2024年
    };
    
    if (rtc.setTime(&time)) {
        Serial.println("时间设置成功");
    }
}

void loop() {
    BM8563_Time currentTime;
    
    if (rtc.getTime(&currentTime)) {
        Serial.printf("20%02d-%02d-%02d %02d:%02d:%02d\n",
                     currentTime.years, currentTime.months, currentTime.days,
                     currentTime.hours, currentTime.minutes, currentTime.seconds);
    }
    
    delay(1000);
}
```

### 闹钟功能

```cpp
void setupAlarm() {
    // 设置闹钟时间
    BM8563_Time alarmTime = {
        .seconds = 0,
        .minutes = 0,
        .hours = 8,   // 早上8点
        .days = 0,
        .weekdays = 0,
        .months = 0,
        .years = 0
    };
    
    // 设置闹钟，mask=0x80 表示只匹配小时
    if (rtc.setAlarm(&alarmTime, 0x80)) {
        Serial.println("闹钟设置成功");
    }
    
    // 启用闹钟中断
    rtc.enableAlarmInterrupt(true);
}

void checkAlarm() {
    if (rtc.getAlarmFlag()) {
        Serial.println("闹钟触发！");
        rtc.clearAlarmFlag();
        // 执行闹钟触发后的操作
    }
}
```

### 定时器功能

```cpp
void setupTimer() {
    // 设置定时器，1Hz频率，60秒
    if (rtc.setTimer(60, BM8563_TIMER_1HZ)) {
        Serial.println("定时器设置成功");
    }
    
    // 启用定时器中断
    rtc.enableTimerInterrupt(true);
}

void checkTimer() {
    if (rtc.getTimerFlag()) {
        Serial.println("定时器触发！");
        rtc.clearTimerFlag();
        // 执行定时器触发后的操作
    }
}
```

### 唤醒定时器

```cpp
void setupWakeup() {
    // 设置300秒（5分钟）后唤醒
    rtc.setupWakeupTimer(300);
    Serial.println("唤醒定时器设置成功");
}
```

## API 参考

### 构造函数
- `BM8563(uint8_t sda_pin, uint8_t scl_pin)` - 创建 BM8563 实例

### 基本方法
- `bool begin()` - 初始化 RTC
- `void reset()` - 复位 RTC
- `bool getTime(BM8563_Time *time)` - 获取当前时间
- `bool setTime(const BM8563_Time *time)` - 设置时间

### 闹钟方法
- `bool setAlarm(const BM8563_Time *alarm_time, uint8_t alarm_mask)` - 设置闹钟
- `bool clearAlarm()` - 清除闹钟
- `bool getAlarmFlag()` - 获取闹钟标志
- `void clearAlarmFlag()` - 清除闹钟标志
- `void enableAlarmInterrupt(bool enable)` - 启用/禁用闹钟中断

### 定时器方法
- `bool setTimer(uint8_t timer_value, uint8_t timer_freq)` - 设置定时器
- `bool clearTimer()` - 清除定时器
- `bool getTimerFlag()` - 获取定时器标志
- `void clearTimerFlag()` - 清除定时器标志
- `void enableTimerInterrupt(bool enable)` - 启用/禁用定时器中断

### 辅助方法
- `void resetInterrupts()` - 复位所有中断
- `void setupWakeupTimer(uint16_t seconds)` - 设置唤醒定时器
- `void setCLKOUTFrequency(uint8_t freq)` - 设置 CLKOUT 频率
- `void enableCLKOUT(bool enable)` - 启用/禁用 CLKOUT
- `bool getPowerFailFlag()` - 获取电源失效标志
- `void stopClock(bool stop)` - 停止/启动时钟

## 常量定义

### 定时器频率
- `BM8563_TIMER_4096HZ` - 4096Hz
- `BM8563_TIMER_64HZ` - 64Hz
- `BM8563_TIMER_1HZ` - 1Hz
- `BM8563_TIMER_1_60HZ` - 1/60Hz

### CLKOUT 频率
- `BM8563_CLKOUT_32768HZ` - 32768Hz
- `BM8563_CLKOUT_1024HZ` - 1024Hz
- `BM8563_CLKOUT_32HZ` - 32Hz
- `BM8563_CLKOUT_1HZ` - 1Hz

## 注意事项

1. BM8563 使用 BCD 码存储时间数据，库会自动进行转换
2. 星期范围：1-7（1=星期一，7=星期日）
3. 年份为两位数（00-99），表示 2000-2099 年
4. 使用中断功能时，需要正确连接 INT 引脚到微控制器的中断引脚
5. 芯片具有备用电池接口，可在主电源断电时保持时间运行

## 技术规格

- 工作电压：1.0V - 5.5V
- 工作温度：-40°C - +85°C
- 时间精度：±3ppm（在25°C时）
- 备用电池电流：典型值 500nA
- I2C 接口：标准模式（100kHz）和快速模式（400kHz）