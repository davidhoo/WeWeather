#include "SHT40.h"

SHT40::SHT40(uint8_t sda_pin, uint8_t scl_pin, uint8_t addr) 
    : _addr(addr), _sda_pin(sda_pin), _scl_pin(scl_pin) {
}

bool SHT40::begin() {
    // 初始化I2C
    Wire.begin(_sda_pin, _scl_pin);
    
    // 检查传感器是否响应
    Wire.beginTransmission(_addr);
    if (Wire.endTransmission() != 0) {
        return false;
    }
    
    // 执行软件复位
    return softReset();
}

bool SHT40::readTemperatureHumidity(float &temperature, float &humidity) {
    // 发送高精度测量命令
    if (!sendCommand(MEAS_HIGHREP)) {
        return false;
    }
    
    // 等待测量完成（高精度测量约8.2ms）
    delay(10);
    
    // 读取6字节数据：温度(2字节) + CRC(1字节) + 湿度(2字节) + CRC(1字节)
    uint8_t data[6];
    if (!readData(data, 6)) {
        return false;
    }
    
    // 检查CRC校验
    if (crc8(&data[0], 2) != data[2] || crc8(&data[3], 2) != data[5]) {
        return false;
    }
    
    // 组合原始数据
    uint16_t rawTemperature = (data[0] << 8) | data[1];
    uint16_t rawHumidity = (data[3] << 8) | data[4];
    
    // 转换为实际温度和湿度值
    convertRawData(rawTemperature, rawHumidity, temperature, humidity);
    
    return true;
}

float SHT40::readTemperature() {
    float temperature, humidity;
    if (readTemperatureHumidity(temperature, humidity)) {
        return temperature;
    }
    return NAN; // 返回非数字表示读取失败
}

float SHT40::readHumidity() {
    float temperature, humidity;
    if (readTemperatureHumidity(temperature, humidity)) {
        return humidity;
    }
    return NAN; // 返回非数字表示读取失败
}

bool SHT40::softReset() {
    if (!sendCommand(SOFTRESET)) {
        return false;
    }
    
    // 等待复位完成
    delay(10);
    
    return true;
}

bool SHT40::readSerialNumber(uint32_t &serialNumber) {
    // 发送读取序列号命令
    if (!sendCommand(READSERIAL)) {
        return false;
    }
    
    // 等待数据准备就绪
    delay(10);
    
    // 读取6字节数据：序列号前半部分(2字节) + CRC(1字节) + 序列号后半部分(2字节) + CRC(1字节)
    uint8_t data[6];
    if (!readData(data, 6)) {
        return false;
    }
    
    // 检查CRC校验
    if (crc8(&data[0], 2) != data[2] || crc8(&data[3], 2) != data[5]) {
        return false;
    }
    
    // 组合序列号
    serialNumber = ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | 
                   ((uint32_t)data[3] << 8) | data[4];
    
    return true;
}

bool SHT40::sendCommand(uint8_t command) {
    Wire.beginTransmission(_addr);
    Wire.write(command);
    return Wire.endTransmission() == 0;
}

bool SHT40::sendCommand(uint16_t command) {
    Wire.beginTransmission(_addr);
    Wire.write(command >> 8);   // 高字节
    Wire.write(command & 0xFF); // 低字节
    return Wire.endTransmission() == 0;
}

bool SHT40::readData(uint8_t *data, uint8_t length) {
    Wire.requestFrom(_addr, length);
    if (Wire.available() != length) {
        return false;
    }
    
    for (uint8_t i = 0; i < length; i++) {
        data[i] = Wire.read();
    }
    
    return true;
}

uint8_t SHT40::crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0xFF; // 初始化值
    
    for (uint8_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 8; bit > 0; --bit) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc = (crc << 1);
            }
        }
    }
    
    return crc;
}

void SHT40::convertRawData(uint16_t rawTemperature, uint16_t rawHumidity, float &temperature, float &humidity) {
    // 温度转换公式: T = -45 + 175 * (rawTemperature / 65535)
    temperature = -45.0f + 175.0f * (float)rawTemperature / 65535.0f;
    
    // 湿度转换公式: RH = -6 + 125 * (rawHumidity / 65535)
    humidity = -6.0f + 125.0f * (float)rawHumidity / 65535.0f;
    
    // 限制湿度范围在0-100%RH之间
    if (humidity > 100.0f) {
        humidity = 100.0f;
    } else if (humidity < 0.0f) {
        humidity = 0.0f;
    }
}