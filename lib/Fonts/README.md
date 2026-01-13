# Fonts 库

字体资源库，提供 WeWeather 项目所需的多种字体文件，包括七段数码管字体和天气符号字体。

## 字体列表

### 1. DSEG7Modern_Bold24pt7b
- 字体名称：DSEG7 Modern Bold
- 字号：24 点
- 风格：七段数码管风格，粗体
- 用途：中等大小时间显示

### 2. DSEG7Modern_Bold28pt7b
- 字体名称：DSEG7 Modern Bold
- 字号：28 点
- 风格：七段数码管风格，粗体
- 用途：较大时间显示

### 3. DSEG7Modern_Bold36pt7b
- 字体名称：DSEG7 Modern Bold
- 字号：36 点
- 风格：七段数码管风格，粗体
- 用途：大时间显示

### 4. DSEG7Modern_Bold42pt7b
- 字体名称：DSEG7 Modern Bold
- 字号：42 点
- 风格：七段数码管风格，粗体
- 用途：超大时间显示

### 5. SevenSegment42pt7b
- 字体名称：Seven Segment
- 字号：42 点
- 风格：传统七段数码管风格
- 用途：时间显示替代方案

### 6. Weather_Symbols_Regular9pt7b
- 字体名称：Weather Symbols Regular
- 字号：9 点
- 风格：天气符号字体
- 用途：天气图标显示

## 使用方法

### 基本字体包含

```cpp
#include "Fonts/DSEG7Modern_Bold42pt7b.h"
#include "Fonts/Weather_Symbols_Regular9pt7b.h"

void setup() {
    // 在显示库中使用字体
    display.setTimeFont(&DSEG7Modern_Bold42pt7b);
    display.setWeatherSymbolFont(&Weather_Symbols_Regular9pt7b);
}
```

### 字体选择建议

#### 时间显示字体
- **小尺寸显示**：DSEG7Modern_Bold24pt7b
- **标准显示**：DSEG7Modern_Bold36pt7b
- **大尺寸显示**：DSEG7Modern_Bold42pt7b

#### 天气符号字体
- **图标显示**：Weather_Symbols_Regular9pt7b
- **小尺寸图标**：Weather_Symbols_Regular9pt7b

## 字体特性

### DSEG7 Modern 字体系列

DSEG7 Modern 字体是专门为七段数码管显示设计的字体，具有以下特点：

1. **数字显示**：0-9 数字字符
2. **冒号支持**：包含时间分隔冒号
3. **高对比度**：清晰易读
4. **现代风格**：简洁美观
5. **多种字号**：24pt, 28pt, 36pt, 42pt

### Weather Symbols 字体

Weather Symbols 字体包含各种天气图标，支持以下天气状况：

1. **晴天**：太阳图标
2. **多云**：云朵图标
3. **雨天**：雨滴图标
4. **雪天**：雪花图标
5. **雷雨**：闪电图标
6. **雾天**：雾气图标
7. **阴天**：阴云图标

## 字体文件格式

所有字体文件都采用 Adafruit GFX 字体格式，包含以下数据结构：

```cpp
// 字体数据结构
typedef struct {
  uint8_t  glyphCount;     // 字符数量
  uint8_t  bitmapWidth;    // 位图宽度
  uint8_t  bitmapHeight;   // 位图高度
  uint8_t  advanceY;       // Y 方向前进量
  uint8_t  first;          // 第一个字符
  uint8_t  last;           // 最后一个字符
  uint8_t  advance;        // 前进量
  uint8_t  bitmapOffset;   // 位图偏移量
  uint8_t  width;          // 字符宽度
  uint8_t  height;         // 字符高度
  uint8_t  xAdvance;       // X 方向前进量
  uint8_t  dX;             // X 偏移
  uint8_t  dY;             // Y 偏移
} GFXglyph;

// 字体信息结构
typedef struct {
  uint8_t   *bitmap;       // 位图数据指针
  GFXglyph  *glyph;        // 字符数据指针
  uint8_t    first;        // 第一个字符
  uint8_t    last;         // 最后一个字符
  uint8_t    yAdvance;     // Y 方向前进量
} GFXfont;
```

## 使用场景

### 1. 时间显示

```cpp
void displayTime() {
    // 设置时间字体
    display.setTimeFont(&DSEG7Modern_Bold36pt7b);
    
    // 显示时间
    display.print("12:34");
}
```

### 2. 天气信息显示

```cpp
void displayWeather() {
    // 设置天气符号字体
    display.setWeatherSymbolFont(&Weather_Symbols_Regular9pt7b);
    
    // 显示天气图标
    display.print("☀");  // 晴天图标
    display.print("☁");  // 多云图标
    display.print("🌧");  // 雨天图标
}
```

### 3. 多字体组合显示

```cpp
void displayCombinedInfo() {
    // 使用大字体显示时间
    display.setFont(&DSEG7Modern_Bold42pt7b);
    display.print("12:34");
    
    // 使用小字体显示天气
    display.setFont(&Weather_Symbols_Regular9pt7b);
    display.print("☀");
    
    // 显示温度
    display.setFont(&FreeMonoBold9pt7b);
    display.print("25°C");
}
```

## 字体性能

### 内存占用

| 字体名称 | 内存占用 | 适用场景 |
|----------|----------|----------|
| DSEG7Modern_Bold24pt7b | ~4KB | 小尺寸显示 |
| DSEG7Modern_Bold28pt7b | ~5KB | 中等显示 |
| DSEG7Modern_Bold36pt7b | ~7KB | 标准显示 |
| DSEG7Modern_Bold42pt7b | ~9KB | 大尺寸显示 |
| SevenSegment42pt7b | ~8KB | 传统风格 |
| Weather_Symbols_Regular9pt7b | ~3KB | 图标显示 |

### 渲染速度

- **位图字体**：渲染速度快，适合嵌入式系统
- **预编译数据**：字体数据预编译，减少运行时计算
- **优化存储**：使用 PROGMEM 存储，节省 RAM

## 字体定制

### 添加新字体

如果需要添加新字体，请遵循以下步骤：

1. **字体转换**：使用 Adafruit GFX 字体转换工具
2. **文件命名**：使用描述性文件名
3. **头文件格式**：遵循现有字体文件格式
4. **测试验证**：确保字体正确显示

### 字体优化建议

1. **字符集精简**：只包含必要的字符
2. **位图压缩**：优化位图数据存储
3. **内存对齐**：优化内存访问效率
4. **缓存机制**：常用字符缓存优化

## 注意事项

1. **内存限制**：注意字体文件大小，避免内存溢出
2. **显示兼容性**：确保字体与显示库兼容
3. **性能影响**：大字体可能影响渲染性能
4. **版权问题**：确保字体使用符合版权要求
5. **存储空间**：考虑 Flash 存储空间限制

## 字体来源

### DSEG7 Modern 字体
- 来源：DSEG 字体项目
- 许可证：SIL Open Font License
- 网址：https://github.com/keshikan/DSEG

### Weather Symbols 字体
- 来源：自定义天气符号字体
- 许可证：开源字体许可证
- 用途：WeWeather 项目专用

## 示例项目

### 完整显示系统

```cpp
#include "GDEY029T94.h"
#include "Fonts/DSEG7Modern_Bold36pt7b.h"
#include "Fonts/Weather_Symbols_Regular9pt7b.h"

GDEY029T94 display(5, 4, 16, 2);

void setup() {
    display.begin();
    
    // 设置字体
    display.setTimeFont(&DSEG7Modern_Bold36pt7b);
    display.setWeatherSymbolFont(&Weather_Symbols_Regular9pt7b);
    
    // 显示时间和天气
    display.showTimeDisplay(currentTime, currentWeather);
}

void loop() {
    // 定期更新显示
    delay(60000);
}
```

## 故障排除

### 常见问题

1. **字体无法显示**
   - 检查字体文件是否正确包含
   - 验证字体指针是否正确传递
   - 确认显示库支持该字体格式

2. **字体显示错位**
   - 检查字体尺寸设置
   - 验证显示坐标计算
   - 调整字体偏移参数

3. **内存不足**
   - 选择更小的字体
   - 优化其他内存使用
   - 考虑字体数据压缩

### 调试建议

1. **字体测试程序**：创建简单的字体测试程序
2. **内存监控**：监控字体加载时的内存使用
3. **性能分析**：分析字体渲染性能
4. **兼容性测试**：在不同显示设备上测试字体