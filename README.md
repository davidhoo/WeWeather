# WeWeather - 微天气

基于 ESP8266 的智能天气显示终端，使用电子墨水屏显示实时天气、时间和环境信息。

## 📋 项目简介

WeWeather 是一个低功耗的智能天气显示设备，集成了多种传感器和显示功能：

- 🌡️ 实时温湿度监测（SHT40 传感器）
- 🌤️ 天气信息显示（高德地图 API）
- ⏰ 精确时间显示（BM8563 RTC）
- 🔋 电池电量监控
- 📱 WiFi 智能配网
- 🖥️ 2.9 寸电子墨水屏显示
- 💤 深度睡眠模式（超低功耗）

## 🔧 硬件配置

### 主控芯片
- **ESP8266** (NodeMCU)

### 显示模块
- **GDEY029T94** 2.9 寸黑白电子墨水屏
- 驱动芯片：SSD1680
- 分辨率：128×296 像素
- 支持局部刷新和快速刷新

### 传感器模块
- **BM8563** RTC 实时时钟模块（I2C）
- **SHT40** 温湿度传感器（I2C）
- **电池监控** 通过 ADC 监测 3.7V 锂电池

### 引脚连接

#### I2C 设备（BM8563 & SHT40）
- SDA: GPIO-2 (D4)
- SCL: GPIO-12 (D6)

#### 电子墨水屏（GDEY029T94）
- CS: D8
- DC: D2
- RST: D0
- BUSY: D1

#### 电池监控
- ADC: A0（通过分压电路连接电池正极）

## 📦 功能特性

### 1. WiFi 智能管理
- 自动连接已保存的 WiFi 网络
- 连接失败自动进入配网模式
- Web 配网界面（AP 模式）
- 配置信息持久化存储（EEPROM）
- 支持自定义 MAC 地址

### 2. 天气信息
- 接入高德地图天气 API
- 显示当前天气状况
- 温度、湿度等信息
- 智能缓存机制，减少 API 调用

### 3. 时间管理
- BM8563 RTC 提供精确时间
- NTP 网络时间同步
- 断网后仍可保持时间准确

### 4. 环境监测
- SHT40 高精度温湿度传感器
- 实时显示室内温湿度

### 5. 电源管理
- 电池电压实时监控
- 电量百分比显示
- 深度睡眠模式（1 分钟唤醒一次）
- RTC 定时器唤醒

### 6. 显示界面
- 大字体时间显示
- 天气图标和信息
- 温湿度数据
- 电池电量指示
- 低功耗刷新

## 🚀 快速开始

### 环境准备

1. 安装 [PlatformIO](https://platformio.org/)
2. 克隆本项目：
```bash
git clone https://github.com/yourusername/WeWeather.git
cd WeWeather
```

### 配置文件设置

1. 复制配置文件模板：
```bash
cp config.h.example config.h
```

2. 编辑 [`config.h`](config.h) 文件，配置以下信息：

```cpp
// 高德地图API配置
#define AMAP_API_KEY "your_amap_api_key_here"
#define CITY_CODE "110108"  // 城市代码，例如：110108为北京海淀区

// WiFi默认配置（可选）
#define DEFAULT_WIFI_SSID "your_wifi_ssid"
#define DEFAULT_WIFI_PASSWORD "your_wifi_password"
#define DEFAULT_MAC_ADDRESS "AA:BB:CC:DD:EE:FF"  // 自定义MAC地址（可选）
```

**注意：** `config.h` 文件已添加到 `.gitignore`，不会被提交到版本库，保护您的隐私信息。

### 获取高德地图 API Key

1. 访问 [高德开放平台](https://lbs.amap.com/)
2. 注册并登录账号
3. 创建应用并申请 Web 服务 API Key
4. 将 API Key 填入 [`config.h`](config.h) 中的 `AMAP_API_KEY`

### 城市代码查询

城市代码（adcode）可以通过以下方式获取：
- 访问 [高德地图城市编码表](https://lbs.amap.com/api/webservice/guide/api/district)
- 或使用高德地图 API 查询接口

### 编译上传

```bash
# 编译项目
pio run

# 上传到设备
pio run --target upload

# 查看串口输出
pio device monitor
```

### 首次配网

1. 设备首次启动会自动进入配网模式
2. 使用手机连接 WiFi 热点：`WeWeather-XXXXXX`
3. 浏览器会自动打开配网页面（或手动访问 `192.168.4.1`）
4. 输入 WiFi 名称和密码
5. 保存后设备自动重启并连接网络

**提示：** 如果已在 [`config.h`](config.h) 中配置了默认 WiFi 信息，设备会先尝试连接默认 WiFi，连接失败后才进入配网模式。

## 📚 项目结构

```
WeWeather/
├── src/
│   └── main.cpp                    # 主程序入口
├── lib/
│   ├── BatteryMonitor/            # 电池监控库
│   │   ├── BatteryMonitor.cpp
│   │   ├── BatteryMonitor.h
│   │   └── README.md
│   ├── BM8563/                    # RTC 实时时钟库
│   │   ├── BM8563.cpp
│   │   └── BM8563.h
│   ├── GDEY029T94/                # 电子墨水屏驱动库
│   │   ├── GDEY029T94.cpp
│   │   └── GDEY029T94.h
│   ├── SHT40/                     # 温湿度传感器库
│   │   ├── SHT40.cpp
│   │   └── SHT40.h
│   ├── TimeManager/               # 时间管理库
│   │   ├── TimeManager.cpp
│   │   └── TimeManager.h
│   ├── WeatherManager/            # 天气管理库
│   │   ├── WeatherManager.cpp
│   │   └── WeatherManager.h
│   ├── WiFiManager/               # WiFi 连接管理库
│   │   ├── WiFiManager.cpp
│   │   └── WiFiManager.h
│   └── Fonts/                     # 显示字体文件
│       ├── DSEG7Modern_Bold28pt7b.h
│       └── Weather_Symbols_Regular9pt7b.h
├── include/                       # 头文件目录
├── test/                          # 测试文件目录
├── config.h.example               # 配置文件模板
├── config.h                       # 配置文件（需自行创建）
├── platformio.ini                 # PlatformIO 配置
└── README.md                      # 项目说明文档
```

## 🔌 依赖库

项目使用以下库（已在 [`platformio.ini`](platformio.ini:15) 中配置）：

- `bblanchon/ArduinoJson` - JSON 解析
- `zinggjm/GxEPD2` - 电子墨水屏驱动
- `ESP8266WebServer` - Web 服务器
- `DNSServer` - DNS 服务器
- `EEPROM` - 配置存储

## ⚙️ 配置说明

### 配置文件（config.h）

项目使用 [`config.h`](config.h) 文件进行配置管理，主要配置项包括：

#### 1. 高德地图 API 配置
```cpp
#define AMAP_API_KEY "your_amap_api_key_here"  // 高德地图 API Key
#define CITY_CODE "110108"                      // 城市代码
```

#### 2. WiFi 默认配置（可选）
```cpp
#define DEFAULT_WIFI_SSID "your_wifi_ssid"           // WiFi 名称
#define DEFAULT_WIFI_PASSWORD "your_wifi_password"  // WiFi 密码
#define DEFAULT_MAC_ADDRESS "AA:BB:CC:DD:EE:FF"      // 自定义 MAC 地址
```

### 深度睡眠时间

在 [`src/main.cpp`](src/main.cpp:18) 中修改睡眠时长：

```cpp
#define DEEP_SLEEP_DURATION 60  // 单位：秒（当前设置为 1 分钟）
```

同时需要修改 RTC 定时器设置（[`src/main.cpp`](src/main.cpp:194)）：

```cpp
rtc.setTimer(60, BM8563_TIMER_1HZ);  // 60 秒后唤醒
```

**注意：** 两处的时间值应保持一致。

### I2C 引脚配置

如需修改 I2C 引脚，在 [`src/main.cpp`](src/main.cpp:21) 中调整：

```cpp
#define SDA_PIN 2   // GPIO-2 (D4)
#define SCL_PIN 12  // GPIO-12 (D6)
```

### 墨水屏引脚配置

墨水屏 SPI 引脚配置（[`src/main.cpp`](src/main.cpp:25)）：

```cpp
#define EPD_CS    D8    // 片选
#define EPD_DC    D2    // 数据/命令
#define EPD_RST   D0    // 复位
#define EPD_BUSY  D1    // 忙碌信号
```

### 电池电压校准

如需校准电池电压读数，可以修改 [`lib/BatteryMonitor/BatteryMonitor.cpp`](lib/BatteryMonitor/BatteryMonitor.cpp) 中的分压系数和电压映射参数。

### WiFi 连接超时

WiFi 连接超时和重试次数可在 [`lib/WiFiManager/WiFiManager.cpp`](lib/WiFiManager/WiFiManager.cpp) 中调整。

## 🔄 系统工作流程

### 启动流程

1. **系统初始化**
   - 初始化串口通信（74880 波特率）
   - 初始化 WeatherManager（天气管理）
   - 初始化 SHT40 温湿度传感器
   - 初始化 GDEY029T94 墨水屏并设置字体
   - 初始化 BM8563 RTC 时钟并清除中断标志

2. **WiFi 连接**
   - 使用 WiFiManager 的智能连接功能
   - 优先尝试连接已保存的 WiFi 或 [`config.h`](config.h) 中配置的默认 WiFi
   - 连接成功：进入正常工作模式
   - 连接失败：自动进入配网模式

3. **配网模式**（WiFi 连接失败时）
   - 创建 AP 热点：`WeWeather-XXXXXX`
   - 在墨水屏显示配网信息（AP 名称和 IP 地址）
   - 启动 Web 配置界面
   - 等待用户通过浏览器配置 WiFi
   - 配置完成后自动重启设备

4. **数据更新**（正常模式）
   - 检查天气数据是否过期（通过 WeatherManager）
   - 如果过期：从网络更新 NTP 时间和天气数据
   - 如果未过期：使用 RTC 缓存的数据

5. **数据采集与显示**
   - 读取 SHT40 温湿度数据
   - 读取电池电压和电量百分比
   - 获取当前时间（从 RTC）
   - 获取天气信息（从缓存或网络）
   - 在墨水屏上显示所有信息

6. **进入深度睡眠**
   - 清除 RTC 中断标志
   - 设置 RTC 定时器（60 秒）
   - ESP8266 进入深度睡眠（功耗 < 20μA）
   - 等待 RTC 定时器唤醒

### 唤醒流程

- RTC 定时器到期后通过 INT 引脚触发硬件复位
- ESP8266 从深度睡眠唤醒
- 重新执行完整的启动流程
- 循环往复，每分钟更新一次显示

## 🔋 功耗优化

### 优化策略

- **深度睡眠模式**：每分钟唤醒一次，睡眠期间功耗 < 20μA
- **电子墨水屏**：断电保持显示，无需持续供电
- **WiFi 智能管理**：数据更新后立即断开，减少功耗
- **RTC 定时器唤醒**：功耗极低（< 1μA），精确定时
- **智能缓存机制**：减少网络请求次数，延长电池寿命
- **快速启动**：优化启动流程，减少唤醒时间

### 预计续航时间

基于 3.7V 2000mAh 锂电池：
- 每分钟唤醒一次
- 每次唤醒约 5-10 秒
- 预计续航：**30-60 天**

*实际续航时间取决于 WiFi 信号强度、更新频率和环境温度等因素。*

## 🐛 故障排除

### 设备无法连接 WiFi
1. 长按复位键，设备会自动进入配网模式
2. 检查 WiFi 密码是否正确
3. 确认路由器支持 2.4GHz 频段

### 时间显示不准确
1. 确保设备已连接网络
2. 检查 NTP 服务器是否可访问
3. 验证 RTC 电池是否正常

### 温湿度读取失败
1. 检查 I2C 连接是否正确
2. 确认 SDA/SCL 引脚配置
3. 验证传感器供电是否正常

### 屏幕显示异常
1. 检查 SPI 引脚连接
2. 确认屏幕型号匹配
3. 尝试完全刷新屏幕


## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

## 📄 许可证

本项目采用 MIT 许可证。详见 [LICENSE](LICENSE) 文件。

## 👨‍💻 作者

- 项目维护者：[David Hu]

## 🙏 致谢

- 高德地图提供天气 API
- GxEPD2 库作者
- PlatformIO 团队
- 所有贡献者

---

⭐ 如果这个项目对你有帮助，请给个 Star！
