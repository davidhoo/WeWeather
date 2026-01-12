# 配置说明

## 配置文件设置

本项目使用 `config.h` 文件进行统一配置管理。该文件包含敏感信息（如 WiFi 密码、API 密钥等），因此已被添加到 `.gitignore` 中，不会提交到 Git 仓库。

### 首次使用步骤

1. 复制配置模板文件：
   ```bash
   cp config.h.example config.h
   ```

2. 编辑 `config.h` 文件，填入你的实际配置信息：
   - WiFi SSID 和密码
   - 高德地图 API Key
   - 城市代码
   - 其他个性化配置

### 配置项说明

#### 硬件配置

- **I2C_SDA_PIN**: I2C 数据引脚（默认：GPIO-2/D4）
- **I2C_SCL_PIN**: I2C 时钟引脚（默认：GPIO-12/D6）
- **EPD_CS_PIN**: 墨水屏片选引脚（默认：D8）
- **EPD_DC_PIN**: 墨水屏数据/命令引脚（默认：D2）
- **EPD_RST_PIN**: 墨水屏复位引脚（默认：D0）
- **EPD_BUSY_PIN**: 墨水屏忙碌信号引脚（默认：D1）

#### 系统配置

- **SERIAL_BAUD_RATE**: 串口波特率（默认：74880）
- **DEEP_SLEEP_SECONDS**: 深度睡眠时间，单位秒（默认：60）
- **RTC_TIMER_SECONDS**: RTC 定时器时间，必须与深度睡眠时间一致（默认：60）
- **DISPLAY_ROTATION**: 显示旋转角度，0=0°, 1=90°, 2=180°, 3=270°（默认：1）

#### API 配置

- **AMAP_API_KEY**: 高德地图 API 密钥
  - 申请地址：https://lbs.amap.com/
  - 需要注册账号并创建应用获取
  
- **CITY_CODE**: 城市代码
  - 例如：110108 为北京海淀区
  - 可在高德地图开放平台查询城市代码
  
- **WEATHER_UPDATE_INTERVAL**: 天气更新间隔，单位秒（默认：1800，即30分钟）

#### WiFi 配置

- **DEFAULT_WIFI_SSID**: WiFi 网络名称
- **DEFAULT_WIFI_PASSWORD**: WiFi 密码
- **DEFAULT_MAC_ADDRESS**: 自定义 MAC 地址（可选）
- **WIFI_CONNECT_TIMEOUT**: WiFi 连接超时时间，单位毫秒（默认：30000，即30秒）

#### 电池配置

- **BATTERY_MIN_VOLTAGE**: 电池最低电压，单位伏特（默认：3.0V）
- **BATTERY_MAX_VOLTAGE**: 电池最高电压，单位伏特（默认：4.2V）

### 注意事项

1. **不要提交 config.h 到 Git**：该文件包含敏感信息，已在 `.gitignore` 中排除
2. **保留 config.h.example**：这是配置模板，可以提交到 Git，方便其他开发者了解需要配置的项目
3. **定期备份你的 config.h**：建议将配置文件备份到安全的地方
4. **团队协作**：团队成员需要各自创建自己的 `config.h` 文件

### 配置文件结构

项目中的配置使用方式：

- [`src/main.cpp`](src/main.cpp:6) - 主程序引用配置
- [`lib/WiFiManager/WiFiManager.cpp`](lib/WiFiManager/WiFiManager.cpp:2) - WiFi 管理器使用配置
- [`lib/WeatherManager/WeatherManager.cpp`](lib/WeatherManager/WeatherManager.cpp:2) - 天气管理器使用配置

所有模块都通过 `#include "../../config.h"` 或 `#include "../config.h"` 引用统一的配置文件。

### 常见问题

**Q: 编译时提示找不到 config.h？**  
A: 请确保已经从 `config.h.example` 复制并创建了 `config.h` 文件。

**Q: 如何获取高德地图 API Key？**  
A: 访问 https://lbs.amap.com/ 注册账号，创建应用后即可获取 API Key。

**Q: 如何查找我所在城市的城市代码？**  
A: 可以在高德地图开放平台的文档中查询，或使用高德的城市编码查询 API。

**Q: 可以不设置自定义 MAC 地址吗？**  
A: 可以，如果不需要自定义 MAC 地址，可以在代码中禁用该功能，或使用默认的 MAC 地址。
