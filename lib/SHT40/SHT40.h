#ifndef SHT40_H
#define SHT40_H

#include <Arduino.h>
#include <Wire.h>

class SHT40 {
public:
    // SHT40 I2C地址
    static const uint8_t SHT40_ADDR = 0x44;  // 默认地址
    
    // 测量命令
    static const uint8_t MEAS_HIGHREP_STRETCH = 0x2C06;  // 高精度测量
    static const uint8_t MEAS_MEDREP_STRETCH = 0x2C0D;   // 中精度测量
    static const uint8_t MEAS_LOWREP_STRETCH = 0x2C10;   // 低精度测量
    static const uint8_t MEAS_HIGHREP = 0xFD;            // 高精度测量（单字节命令）
    static const uint8_t MEAS_MEDREP = 0xF6;             // 中精度测量（单字节命令）
    static const uint8_t MEAS_LOWREP = 0xE0;             // 低精度测量（单字节命令）
    
    // 其他命令
    static const uint8_t READSERIAL = 0x89;              // 读取序列号
    static const uint8_t SOFTRESET = 0x94;               // 软件复位
    
    // 构造函数
    SHT40(uint8_t sda_pin, uint8_t scl_pin, uint8_t addr = SHT40_ADDR);
    
    // 初始化传感器
    bool begin();
    
    // 读取温湿度数据
    bool readTemperatureHumidity(float &temperature, float &humidity);
    
    // 读取温度（摄氏度）
    float readTemperature();
    
    // 读取湿度（%RH）
    float readHumidity();
    
    // 软件复位
    bool softReset();
    
    // 读取序列号
    bool readSerialNumber(uint32_t &serialNumber);

private:
    uint8_t _addr;
    uint8_t _sda_pin;
    uint8_t _scl_pin;
    
    // 发送命令并等待测量完成
    bool sendCommand(uint8_t command);
    bool sendCommand(uint16_t command);
    
    // 读取测量数据
    bool readData(uint8_t *data, uint8_t length);
    
    // 计算CRC校验
    uint8_t crc8(const uint8_t *data, uint8_t len);
    
    // 转换原始数据为温度和湿度
    void convertRawData(uint16_t rawTemperature, uint16_t rawHumidity, float &temperature, float &humidity);
};

#endif // SHT40_H