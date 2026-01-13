# GDEY029T94 库

GDEY029T94 2.9 英寸电子墨水屏显示驱动库，专为 WeWeather 项目设计，用于显示时间、天气信息和系统状态。

## 功能特性

- 2.9 英寸电子墨水屏（296x128 像素）
- 黑白双色显示
- 低功耗设计，仅在内容更新时耗电
- 支持时间显示
- 支持天气信息显示
- 支持电池电量显示
- 支持配置模式显示
- 可自定义字体
- 8像素对齐优化

## 硬件连接

- VCC → 3.3V
- GND → GND
- DIN → SPI MOSI 引脚
- CLK → SPI SCK 引脚
- CS → 片选引脚（可配置）
- DC → 数据/命令选择引脚（可配置）
- RST → 复位引脚（可配置）
- BUSY → 忙碌状态引脚（可配置）

## 使用方法

### 基本初始化

```cpp
#include "GDEY029T94.h"
#include "TimeManager.h"
#include "WeatherManager.h"

// 创建显示实例，指定引脚
GDEY029T94 display(5, 4, 16, 2);  // CS=5, DC=4, RST=16, BUSY=2

void setup() {
    Serial.begin(115200);
    
    // 初始化显示屏
    display.begin();
    
    // 设置旋转方向（0, 90, 180, 270度）
    display.setRotation(0);
}
```

### 显示时间和天气信息

```cpp
void loop() {
    // 获取当前时间
    DateTime currentTime;
    if (timeManager.getCurrentTime(&currentTime)) {
        // 获取天气信息
        WeatherInfo weatherInfo;
        if (weatherManager.getCurrentWeather(&weatherInfo)) {
            // 获取传感器数据
            float temperature = 25.5;  // 从传感器获取
            float humidity = 60.0;     // 从传感器获取
            float battery = 85.0;      // 从电池监控获取
            
            // 显示完整信息
            display.showTimeDisplay(currentTime, weatherInfo, 
                                   temperature, humidity, battery);
        }
    }
    
    delay(60000);  // 每分钟更新一次
}
```

### 显示配置模式

```cpp
void enterConfigMode() {
    const char* apName = "WeWeather-Config";
    const char* apIP = "192.168.4.1";
    
    // 显示配置界面
    display.showConfigDisplay(apName, apIP);
}
```

### 自定义字体

```cpp
void setupCustomFonts() {
    // 设置时间显示字体
    display.setTimeFont(&DSEG7Modern_Bold42pt7b);
    
    // 设置天气符号字体
    display.setWeatherSymbolFont(&Weather_Symbols_Regular9pt7b);
}
```

## API 参考

### 构造函数
- `GDEY029T94(uint8_t cs, uint8_t dc, uint8_t rst, uint8_t busy)` - 创建显示实例

### 基本方法
- `void begin()` - 初始化显示屏
- `void setRotation(int rotation)` - 设置屏幕旋转方向（0, 90, 180, 270）

### 显示方法
- `void showTimeDisplay(const DateTime& currentTime, const WeatherInfo& currentWeather, float temperature = NAN, float humidity = NAN, float batteryPercentage = NAN)` - 显示时间和天气信息
- `void showConfigDisplay(const char* apName, const char* apIP)` - 显示配置模式界面

### 字体设置
- `void setTimeFont(const GFXfont* font)` - 设置时间显示字体
- `void setWeatherSymbolFont(const GFXfont* font)` - 设置天气符号字体

### 辅助方法
- `int alignToPixel8(int x)` - 8像素对齐辅助函数

## 显示内容说明

### 时间显示模式
- 大字体显示当前时间
- 显示日期信息
- 显示天气图标和描述
- 显示温度和湿度
- 显示电池电量图标

### 配置模式显示
- 显示 AP 名称
- 显示 IP 地址
- 显示配置说明信息

## 字体支持

库支持自定义字体，推荐使用以下字体：

- 时间显示：DSEG7Modern 系列（24pt, 28pt, 36pt, 42pt）
- 天气符号：Weather_Symbols_Regular9pt7b
- 其他文本：FreeMonoBold9pt7b

## 性能优化

1. **8像素对齐**：使用 `alignToPixel8()` 函数确保文本正确对齐
2. **刷新频率**：电子墨水屏刷新较慢，建议适当控制更新频率
3. **功耗管理**：仅在内容变化时刷新屏幕

## 技术规格

- 显示尺寸：2.9 英寸
- 分辨率：296 × 128 像素
- 显示颜色：黑白双色
- 接口类型：SPI
- 工作电压：3.3V
- 刷新时间：约 2-3 秒
- 视角：>170°
- 工作温度：-20°C 到 +70°C

## 注意事项

1. 电子墨水屏刷新次数有限，避免频繁刷新
2. 在更新显示内容时确保有足够的电源供应
3. 长时间显示静态内容不会产生残影
4. 使用前确保正确连接所有引脚
5. 在低温环境下刷新速度可能变慢

## 依赖库

- GxEPD2：电子墨水屏驱动基础库
- TimeManager：时间管理库
- WeatherManager：天气数据管理库
- Adafruit_GFX：图形绘制基础库