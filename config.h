#ifndef CONFIG_H
#define CONFIG_H

// ==================== 硬件配置 ====================

// I2C 引脚配置 (BM8563 RTC & SHT40 传感器)
#define I2C_SDA_PIN 2   // GPIO-2 (D4)
#define I2C_SCL_PIN 12  // GPIO-12 (D6)

// 墨水屏 SPI 引脚配置 (GDEY029T94)
#define EPD_CS_PIN   D8  // 片选
#define EPD_DC_PIN   D2  // 数据/命令
#define EPD_RST_PIN  D0  // 复位
#define EPD_BUSY_PIN D1  // 忙碌信号

// RXD 引脚配置 (用于配置模式触发)
#define RXD_PIN 3  // GPIO-3 (RXD)

// ==================== 系统配置 ====================

// 串口波特率 (ESP8266 ROM bootloader 默认波特率)
#define SERIAL_BAUD_RATE 74880

// 深度睡眠配置
#define DEEP_SLEEP_SECONDS 60  // 深度睡眠时间（秒），1分钟唤醒一次
#define RTC_TIMER_SECONDS  60  // RTC 定时器时间（必须与深度睡眠时间一致）

// 显示配置
#define DISPLAY_ROTATION 1  // 旋转角度：0=0°, 1=90°, 2=180°, 3=270°

// ==================== API 配置 ====================

// 高德地图 API 配置
// 申请地址: https://lbs.amap.com/
#define DEFAULT_AMAP_API_KEY "b4bed4011e9375d01423a45fba58e836"
#define DEFAULT_CITY_CODE "110108"  // 城市代码，例如：110108为北京海淀区

// 天气更新间隔（秒）
#define WEATHER_UPDATE_INTERVAL 1800  // 30分钟

// ==================== WiFi 配置 ====================

// WiFi 默认配置（可选，如不配置则首次启动进入配网模式）
#define DEFAULT_WIFI_SSID "Sina Plaza Office"
#define DEFAULT_WIFI_PASSWORD "urtheone"

// 自定义 MAC 地址（可选）
#define DEFAULT_MAC_ADDRESS "14:2B:2F:EC:0B:04"

// MAC 地址设置选项
#define ENABLE_CUSTOM_MAC true  // 设置为 false 可禁用自定义MAC地址功能
#define MAC_SET_EARLY_BOOT true // 在系统启动早期尝试设置MAC地址

// WiFi 连接超时（毫秒）
#define WIFI_CONNECT_TIMEOUT 30000  // 30秒

// ==================== 电池配置 ====================

// 电池电压范围（用于电量百分比计算）
#define BATTERY_MIN_VOLTAGE 3.0  // 最低电压（V）
#define BATTERY_MAX_VOLTAGE 4.2  // 最高电压（V）

#endif // CONFIG_H
