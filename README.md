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

### 配置 API

编辑 [`src/main.cpp`](src/main.cpp:42)，修改高德地图 API 配置：

```cpp
const char* AMAP_API_KEY = "your_amap_api_key";
String cityCode = "110108";  // 修改为你的城市代码
```

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

## 📚 项目结构

```
WeWeather/
├── src/
│   └── main.cpp                    # 主程序
├── lib/
│   ├── BatteryMonitor/            # 电池监控库
│   ├── BM8563/                    # RTC 时钟库
│   ├── GDEY029T94/                # 电子墨水屏驱动
│   ├── SHT40/                     # 温湿度传感器库
│   ├── TimeManager/               # 时间管理库
│   ├── WeatherManager/            # 天气管理库
│   ├── WiFiManager/               # WiFi 管理库
│   └── Fonts/                     # 字体文件
├── test/                          # 测试文件
├── platformio.ini                 # PlatformIO 配置
└── README.md                      # 项目说明
```

## 🔌 依赖库

项目使用以下库（已在 [`platformio.ini`](platformio.ini:15) 中配置）：

- `bblanchon/ArduinoJson` - JSON 解析
- `zinggjm/GxEPD2` - 电子墨水屏驱动
- `ESP8266WebServer` - Web 服务器
- `DNSServer` - DNS 服务器
- `EEPROM` - 配置存储

## ⚙️ 配置说明

### 深度睡眠时间

在 [`src/main.cpp`](src/main.cpp:17) 中修改：

```cpp
#define DEEP_SLEEP_DURATION 60  // 单位：秒
```

### 电池电压校准

如需校准电池电压，修改 [`lib/BatteryMonitor/BatteryMonitor.cpp`](lib/BatteryMonitor/BatteryMonitor.cpp) 中的分压系数。

### WiFi 连接超时

在 [`lib/WiFiManager/WiFiManager.cpp`](lib/WiFiManager/WiFiManager.cpp) 中调整超时参数。

## 🔋 功耗优化

- 使用深度睡眠模式，每分钟唤醒一次
- 电子墨水屏断电保持显示
- WiFi 连接后立即进入睡眠
- RTC 定时器唤醒，功耗极低

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

## 📝 开发计划

- [ ] 添加多城市天气支持
- [ ] 实现天气预报显示
- [ ] 增加更多传感器支持
- [ ] OTA 固件升级功能
- [ ] 移动 APP 配置界面
- [ ] 历史数据记录和图表

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

## 📄 许可证

本项目采用 MIT 许可证。详见 [LICENSE](LICENSE) 文件。

## 👨‍💻 作者

- 项目维护者：[Your Name]

## 🙏 致谢

- 高德地图提供天气 API
- GxEPD2 库作者
- PlatformIO 团队
- 所有贡献者

---

⭐ 如果这个项目对你有帮助，请给个 Star！
