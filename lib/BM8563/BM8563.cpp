#include "BM8563.h"

BM8563::BM8563(uint8_t sda_pin, uint8_t scl_pin) {
    _sda_pin = sda_pin;
    _scl_pin = scl_pin;
}

bool BM8563::begin() {
    Wire.begin(_sda_pin, _scl_pin);
    
    // 检查芯片是否响应
    Wire.beginTransmission(BM8563_I2C_ADDR);
    if (Wire.endTransmission() != 0) {
        return false;
    }
    
    // 初始化控制寄存器
    uint8_t ctrl1 = 0x00;  // 普通模式，时钟运行
    uint8_t ctrl2 = BM8563_TI_TP;  // 设置TI/TP=1，使用脉冲模式
    
    writeRegister(BM8563_CTRL_STATUS1, ctrl1);
    writeRegister(BM8563_CTRL_STATUS2, ctrl2);
    
    return true;
}

void BM8563::reset() {
    uint8_t ctrl1 = BM8563_TESTC;  // 启用电源复位功能
    writeRegister(BM8563_CTRL_STATUS1, ctrl1);
    
    delay(10);
    
    ctrl1 = 0x00;  // 普通模式
    writeRegister(BM8563_CTRL_STATUS1, ctrl1);
}

bool BM8563::getTime(BM8563_Time *time) {
    uint8_t buffer[7];
    
    if (!readRegisters(BM8563_SECONDS, buffer, 7)) {
        return false;
    }
    
    time->seconds = bcdToDec(buffer[0] & 0x7F);  // 去除VL位
    time->minutes = bcdToDec(buffer[1] & 0x7F);
    time->hours = bcdToDec(buffer[2] & 0x3F);
    time->days = bcdToDec(buffer[3] & 0x3F);
    time->weekdays = buffer[4] & 0x07;
    time->months = bcdToDec(buffer[5] & 0x1F);
    time->years = bcdToDec(buffer[6]);
    
    return true;
}

bool BM8563::setTime(const BM8563_Time *time) {
    uint8_t buffer[7];
    
    buffer[0] = decToBcd(time->seconds);  // VL位默认为0
    buffer[1] = decToBcd(time->minutes);
    buffer[2] = decToBcd(time->hours);
    buffer[3] = decToBcd(time->days);
    buffer[4] = time->weekdays & 0x07;
    buffer[5] = decToBcd(time->months);
    buffer[6] = decToBcd(time->years);
    
    return writeRegisters(BM8563_SECONDS, buffer, 7);
}

bool BM8563::setAlarm(const BM8563_Time *alarm_time, uint8_t alarm_mask) {
    uint8_t buffer[4];
    
    // 设置分钟报警
    buffer[0] = (alarm_mask & 0x01) ? (BM8563_AE | decToBcd(alarm_time->minutes)) : decToBcd(alarm_time->minutes);
    
    // 设置小时报警
    buffer[1] = (alarm_mask & 0x02) ? (BM8563_AE | decToBcd(alarm_time->hours)) : decToBcd(alarm_time->hours);
    
    // 设置日报警
    buffer[2] = (alarm_mask & 0x04) ? (BM8563_AE | decToBcd(alarm_time->days)) : decToBcd(alarm_time->days);
    
    // 设置星期报警
    buffer[3] = (alarm_mask & 0x08) ? (BM8563_AE | (alarm_time->weekdays & 0x07)) : (alarm_time->weekdays & 0x07);
    
    return writeRegisters(BM8563_ALARM_MINUTES, buffer, 4);
}

bool BM8563::clearAlarm() {
    uint8_t buffer[4];
    
    // 禁用所有报警
    buffer[0] = BM8563_AE;
    buffer[1] = BM8563_AE;
    buffer[2] = BM8563_AE;
    buffer[3] = BM8563_AE;
    
    return writeRegisters(BM8563_ALARM_MINUTES, buffer, 4);
}

bool BM8563::getAlarmFlag() {
    uint8_t value;
    if (!readRegister(BM8563_CTRL_STATUS2, &value)) {
        return false;
    }
    return (value & BM8563_AF) != 0;
}

void BM8563::clearAlarmFlag() {
    uint8_t value;
    readRegister(BM8563_CTRL_STATUS2, &value);
    value &= ~BM8563_AF;
    writeRegister(BM8563_CTRL_STATUS2, value);
}

void BM8563::enableAlarmInterrupt(bool enable) {
    uint8_t value;
    readRegister(BM8563_CTRL_STATUS2, &value);
    
    if (enable) {
        value |= BM8563_AIE;
    } else {
        value &= ~BM8563_AIE;
    }
    
    writeRegister(BM8563_CTRL_STATUS2, value);
}

bool BM8563::setTimer(uint8_t timer_value, uint8_t timer_freq) {
    // 设置定时器控制寄存器
    uint8_t timer_ctrl = BM8563_TE | (timer_freq & 0x03);
    writeRegister(BM8563_TIMER_CTRL, timer_ctrl);
    
    // 设置定时器值
    return writeRegister(BM8563_TIMER, timer_value);
}

bool BM8563::clearTimer() {
    // 禁用定时器
    uint8_t timer_ctrl = 0x00;
    writeRegister(BM8563_TIMER_CTRL, timer_ctrl);
    
    // 清除定时器标志
    clearTimerFlag();
    
    return true;
}

bool BM8563::getTimerFlag() {
    uint8_t value;
    if (!readRegister(BM8563_CTRL_STATUS2, &value)) {
        return false;
    }
    return (value & BM8563_TF) != 0;
}

void BM8563::clearTimerFlag() {
    uint8_t value;
    readRegister(BM8563_CTRL_STATUS2, &value);
    value &= ~BM8563_TF;
    writeRegister(BM8563_CTRL_STATUS2, value);
}

void BM8563::enableTimerInterrupt(bool enable) {
    uint8_t value;
    readRegister(BM8563_CTRL_STATUS2, &value);
    
    if (enable) {
        value |= BM8563_TIE;
    } else {
        value &= ~BM8563_TIE;
    }
    
    writeRegister(BM8563_CTRL_STATUS2, value);
}

/**
 * @brief 重置所有中断标志和禁用中断
 * @note 用于清除可能导致 INT 引脚拉低的状态
 */
void BM8563::resetInterrupts() {
    clearTimerFlag();
    clearAlarmFlag();
    enableTimerInterrupt(false);
    enableAlarmInterrupt(false);
}

/**
 * @brief 配置深度睡眠唤醒定时器
 * @param seconds 睡眠时间（秒）
 * @note 自动清除中断标志并设置定时器
 */
void BM8563::setupWakeupTimer(uint16_t seconds) {
    resetInterrupts();
    setTimer(seconds, BM8563_TIMER_1HZ);
    enableTimerInterrupt(true);
}

void BM8563::setCLKOUTFrequency(uint8_t freq) {
    uint8_t value;
    readRegister(BM8563_CLKOUT, &value);
    
    value &= ~0x03;  // 清除频率位
    value |= (freq & 0x03);  // 设置新的频率
    
    writeRegister(BM8563_CLKOUT, value);
}

void BM8563::enableCLKOUT(bool enable) {
    uint8_t value;
    readRegister(BM8563_CLKOUT, &value);
    
    if (enable) {
        value |= BM8563_FE;
    } else {
        value &= ~BM8563_FE;
    }
    
    writeRegister(BM8563_CLKOUT, value);
}

bool BM8563::getPowerFailFlag() {
    uint8_t value;
    if (!readRegister(BM8563_SECONDS, &value)) {
        return false;
    }
    return (value & BM8563_VL) != 0;
}

void BM8563::stopClock(bool stop) {
    uint8_t value;
    readRegister(BM8563_CTRL_STATUS1, &value);
    
    if (stop) {
        value |= BM8563_STOP;
    } else {
        value &= ~BM8563_STOP;
    }
    
    writeRegister(BM8563_CTRL_STATUS1, value);
}

// 私有辅助函数
uint8_t BM8563::bcdToDec(uint8_t bcd) {
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}

uint8_t BM8563::decToBcd(uint8_t dec) {
    return ((dec / 10) << 4) | (dec % 10);
}

bool BM8563::readRegister(uint8_t reg, uint8_t *value) {
    Wire.beginTransmission(BM8563_I2C_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) {
        return false;
    }
    
    Wire.requestFrom(BM8563_I2C_ADDR, (uint8_t)1);
    if (Wire.available() != 1) {
        return false;
    }
    
    *value = Wire.read();
    return true;
}

bool BM8563::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(BM8563_I2C_ADDR);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
}

bool BM8563::readRegisters(uint8_t reg, uint8_t *buffer, uint8_t len) {
    Wire.beginTransmission(BM8563_I2C_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) {
        return false;
    }
    
    Wire.requestFrom(BM8563_I2C_ADDR, len);
    if (Wire.available() != len) {
        return false;
    }
    
    for (uint8_t i = 0; i < len; i++) {
        buffer[i] = Wire.read();
    }
    
    return true;
}

bool BM8563::writeRegisters(uint8_t reg, uint8_t *buffer, uint8_t len) {
    Wire.beginTransmission(BM8563_I2C_ADDR);
    Wire.write(reg);
    
    for (uint8_t i = 0; i < len; i++) {
        Wire.write(buffer[i]);
    }
    
    return Wire.endTransmission() == 0;
}