#ifndef BM8563_H
#define BM8563_H

#include <Arduino.h>
#include <Wire.h>

// BM8563 I2C地址
#define BM8563_I2C_ADDR 0x51  // 0xA2 >> 1

// 寄存器地址
#define BM8563_CTRL_STATUS1    0x00
#define BM8563_CTRL_STATUS2    0x01
#define BM8563_SECONDS         0x02
#define BM8563_MINUTES         0x03
#define BM8563_HOURS           0x04
#define BM8563_DAYS            0x05
#define BM8563_WEEKDAYS        0x06
#define BM8563_MONTHS          0x07
#define BM8563_YEARS           0x08
#define BM8563_ALARM_MINUTES   0x09
#define BM8563_ALARM_HOURS     0x0A
#define BM8563_ALARM_DAYS      0x0B
#define BM8563_ALARM_WEEKDAYS  0x0C
#define BM8563_CLKOUT          0x0D
#define BM8563_TIMER_CTRL      0x0E
#define BM8563_TIMER           0x0F

// 控制状态寄存器1位定义
#define BM8563_TEST1   0x80
#define BM8563_STOP    0x20
#define BM8563_TESTC   0x08

// 控制状态寄存器2位定义
#define BM8563_TI_TP   0x10
#define BM8563_AF      0x08
#define BM8563_TF      0x04
#define BM8563_AIE     0x02
#define BM8563_TIE     0x01

// 秒寄存器位定义
#define BM8563_VL      0x80

// 月份寄存器位定义
#define BM8563_C       0x80

// 报警寄存器位定义
#define BM8563_AE      0x80

// CLKOUT寄存器位定义
#define BM8563_FE      0x80

// 定时器控制寄存器位定义
#define BM8563_TE      0x80

// 定时器频率选择
#define BM8563_TIMER_4096HZ  0x00
#define BM8563_TIMER_64HZ    0x01
#define BM8563_TIMER_1HZ     0x02
#define BM8563_TIMER_1_60HZ  0x03

// CLKOUT频率选择
#define BM8563_CLKOUT_32768HZ 0x00
#define BM8563_CLKOUT_1024HZ  0x01
#define BM8563_CLKOUT_32HZ    0x02
#define BM8563_CLKOUT_1HZ     0x03

// 时间结构体
typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t days;
    uint8_t weekdays;
    uint8_t months;
    uint8_t years;
} BM8563_Time;

class BM8563 {
public:
    BM8563(uint8_t sda_pin, uint8_t scl_pin);
    
    // 基本功能
    bool begin();
    void reset();
    
    // 时间读写
    bool getTime(BM8563_Time *time);
    bool setTime(const BM8563_Time *time);
    
    // 闹钟功能
    bool setAlarm(const BM8563_Time *alarm_time, uint8_t alarm_mask);
    bool clearAlarm();
    bool getAlarmFlag();
    void clearAlarmFlag();
    void enableAlarmInterrupt(bool enable);
    
    // 定时器功能
    bool setTimer(uint8_t timer_value, uint8_t timer_freq);
    bool clearTimer();
    bool getTimerFlag();
    void clearTimerFlag();
    void enableTimerInterrupt(bool enable);
    
    // 辅助方法（用于简化常用操作）
    void resetInterrupts();
    void setupWakeupTimer(uint16_t seconds);
    
    // CLKOUT功能
    void setCLKOUTFrequency(uint8_t freq);
    void enableCLKOUT(bool enable);
    
    // 其他功能
    bool getPowerFailFlag();
    void stopClock(bool stop);
    
private:
    uint8_t _sda_pin;
    uint8_t _scl_pin;
    
    // 内部辅助函数
    uint8_t bcdToDec(uint8_t bcd);
    uint8_t decToBcd(uint8_t dec);
    bool readRegister(uint8_t reg, uint8_t *value);
    bool writeRegister(uint8_t reg, uint8_t value);
    bool readRegisters(uint8_t reg, uint8_t *buffer, uint8_t len);
    bool writeRegisters(uint8_t reg, uint8_t *buffer, uint8_t len);
};

#endif // BM8563_H