# LCD4Linux 中文显示支持

为 LCD4Linux 添加 TrueType/FreeType 中文字体渲染支持。

## 功能特性

- ✅ TrueType 中文字体渲染支持
- ✅ 所有图形驱动支持中文显示
- ✅ 所有驱动已编译
- ✅ 支持 UTF-8 中文显示
- ✅ Graph 组件 (AIDA64 风格折线图)
- ✅ Arc 组件 (AIDA64 风格弧形仪表盘)

## 预编译版本

下载 Release: [lcd4linux-PC-64](https://github.com/netusb/lcd4linux-display-chinese/releases)

```bash
# 解压使用
tar -xzvf lcd4linux-PC-64.tar.gz
./lcd4linux-PC-64 -f /path/to/config.conf
```

## 编译依赖

### Ubuntu/Debian

```bash
# 安装编译工具
apt-get install -y build-essential autoconf automake libtool gettext

# 安装驱动依赖
apt-get install -y \
    libvncserver-dev \
    libfreetype6-dev \
    libgd-dev \
    libjpeg-dev \
    libusb-1.0-0-dev \
    libusb-dev \
    libx11-dev \
    libdbus-1-dev

# 可选依赖
apt-get install -y \
    libcurl4-openssl-dev \
    libimlib2-dev \
    libpng-dev
```

### Fedora/RHEL

```bash
yum install -y gcc make autoconf automake libtool gettext

yum install -y \
    libvncserver-devel \
    freetype-devel \
    gd-devel \
    libjpeg-turbo-devel \
    libusb-devel \
    libusb1-devel \
    libX11-devel \
    dbus-devel
```

## 编译步骤

```bash
# 1. 进入源码目录
cd lcd4linux-0.11.0~svn1203

# 2. 生成配置文件
./bootstrap

# 3. 配置 (启用所有驱动)
./configure --with-drivers=all

# 4. 编译
make -j$(nproc)

# 5. 安装
make install
```

或指定特定驱动：

```bash
./configure --with-drivers=VNC,DPF
```

## 配置示例

### VNC 显示配置 (完整示例)

```conf
Display VNC {
    Driver       'VNC'
    Font         '/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc'
    FontSize     '24'
    Port         '5900'
    Xres         '300'
    Yres         '400'
    Bpp          '4'
}

Variables {
    tick 500
}

# CPU 使用率折线图
Widget CPU_Graph {
    class 'Graph'
    expression proc_stat::cpu('busy', 500)
    min 0
    max 100
    width 8
    height 2
    points 500
    style 0
    grid 3
    value 1
    value_size 8
    value_precision 0
    value_unit '%'
    value_bg_color '00000000'    # 透明背景
    direction 0
    update 500
    line_color '00ff00'
    fill_color '00440080'
    bg_color 'FF8000'
    grid_color '333333'
    text_color 'FFFFFF'
}

# CPU 使用率弧形仪表盘
Widget CPU_Arc {
    class 'Arc'
    expression proc_stat::cpu('busy', 500)
    min 0
    max 100
    diameter 100
    thickness 6
    show_background 1
    background_color '222222'
    show_value 1
    value_text_size 16
    value_text_color 'ffffff'
    value_precision 0
    value_unit ''
    show_needle 1
    needle_length 23
    needle_width 2
    needle_color 'ff0000'
    center_color '444444'
    reverse 1
    update 500
    limit_1 33
    limit_2 66
    limit_3 90
    arc_color_min '00ff00'
    arc_color_1 'ffff00'
    arc_color_2 'ff8800'
    arc_color_3 'ff0000'
    back_color_min '00000000'
    back_color_1 '00000000'
    back_color_2 '00000000'
    back_color_3 '00000000'
}

# 时间显示
Widget Time {
    class 'Text'
    expression strftime('时间 %H:%M:%S', time())
    width 16
    speed 1000
    fontsize 18
    font '/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc'
    Foreground '00FF00'
    Background '0000CD00'
}

Layout Main {
    Layer 0 {
        Row1 { Col1 'Time' }
        Row6 { Col4 'CPU_Graph' }
        Row6 { Col4 'CPU_Arc' }
    }
}

Display 'VNC'
Layout 'Main'
```

### DPF 显示配置

```conf
Display DPF {
    Driver       'DPF'
    Port         'usb0'
    Font         '/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc'
    FontSize     '24'        # 可选: 8-64
}

Layout Main {
    Row1 { Col1 'Test' }
}

Display 'DPF'
Layout 'Main'
```

## Graph 组件 (折线图)

Graph 组件用于显示数据的历史变化趋势，类似于 AIDA64 的性能监控折线图。

### 配置参数

| 参数 | 说明 | 默认值 |
|------|------|--------|
| expression | 数据表达式 | 必需 |
| min | 最小值 | 0 |
| max | 最大值 | 100 |
| update | 更新间隔(毫秒) | 1000 |
| points | 数据点数量 | 50 |
| width | 图表宽度(像素) | 40 |
| height | 图表高度(像素) | 20 |
| style | 样式 (0=线, 1=填充) | 0 |
| direction | 方向 (0=左到右, 1=右到左) | 0 |
| value | 显示数值 (0/1) | 1 |
| value_size | 数值文字大小(像素) | 10 |
| value_precision | 数值小数位数 (0,1,2) | 0 |
| value_unit | 数值单位后缀 | "%" |
| value_bg_color | 数值背景颜色(RGBA) | 透明 |
| line_color | 线条颜色 | #00FF00 |
| fill_color | 填充颜色 | #008000 |
| bg_color | 背景颜色 | #000000 |
| grid_color | 网格颜色 | #404040 |
| text_color | 文字颜色 | #FFFFFF |

### 使用示例

```conf
Widget CPU_Graph {
    class 'Graph'
    expression proc_stat::cpu('busy', 500)
    width 8
    height 2
    min 0
    max 100
    update 500
    points 500
    style 0
    value 1
    value_size 8
    value_precision 0
    value_unit '%'
    value_bg_color '00000000'    # 透明背景
    line_color '00ff00'
    bg_color 'FF8000'
}
```

## Arc 组件 (弧形仪表盘)

Arc 组件用于显示单个数值的 AIDA64 风格弧形仪表盘，支持 180° 半圆显示和彩色渐变。

### 配置参数

| 参数 | 说明 | 默认值 |
|------|------|--------|
| expression | 数据表达式 | 必需 |
| min | 最小值 | 0 |
| max | 最大值 | 100 |
| update | 更新间隔(毫秒) | 1000 |
| diameter | 直径(像素) | 100 |
| thickness | 弧线厚度 | 6 |
| reverse | 方向 (0=右到左, 1=左到右) | 0 |
| show_background | 显示背景 (0/1) | 1 |
| background_color | 背景颜色 | #222222 |
| show_value | 显示数值 (0/1) | 1 |
| value_text_size | 数值文字大小(像素) | 12 |
| value_text_color | 数值文字颜色 | #FFFFFF |
| value_precision | 数值小数位数 (0,1,2) | 0 |
| value_unit | 数值单位后缀 | "" |
| show_needle | 显示指针 (0/1) | 1 |
| needle_length | 指针长度(像素) | 0(自动) |
| needle_width | 指针宽度(像素) | 2 |
| needle_color | 指针颜色 | #FF0000 |
| center_color | 中心点颜色 | #444444 |
| arc_color_min | 第一段弧线颜色 | #00FF00 |
| arc_color_1 | 第二段弧线颜色 | #FFFF00 |
| arc_color_2 | 第三段弧线颜色 | #FF8800 |
| arc_color_3 | 第四段弧线颜色 | #FF0000 |

### 使用示例

```conf
Widget CPU_Arc {
    class 'Arc'
    expression proc_stat::cpu('busy', 500)
    min 0
    max 100
    diameter 100
    thickness 6
    reverse 1              # 1=左到右, 0=右到左
    show_background 1
    background_color '222222'
    show_value 1
    value_text_size 16
    value_text_color 'ffffff'
    value_precision 0
    value_unit ''
    show_needle 1
    needle_length 23
    needle_width 2
    needle_color 'ff0000'
    center_color '444444'
    arc_color_min '00ff00'  # 绿色
    arc_color_1 'ffff00'    # 黄色
    arc_color_2 'ff8800'    # 橙色
    arc_color_3 'ff0000'    # 红色
}
```

## Ring 组件 (环形进度条)

Ring 组件用于显示单个数值的 360° 环形进度条，类似于 AIDA64 的圆形传感器显示。

### 配置参数

| 参数 | 说明 | 默认值 |
|------|------|--------|
| expression | 数据表达式 | 必需 |
| min | 最小值 | 0 |
| max | 最大值 | 100 |
| update | 更新间隔(毫秒) | 1000 |
| diameter | 直径(像素) | 100 |
| thickness | 环形粗细(像素) | 10 |
| start_angle | 起始角度 (0=右, 90=上, 180=左, 270=下) | 270 |
| show_background | 显示背景环 (0/1) | 1 |
| background_color | 背景环颜色 | #282828 |
| show_value | 显示数值 (0/1) | 1 |
| value_text_size | 数值文字大小(像素) | 16 |
| value_text_color | 数值文字颜色 | #FFFFFF |
| value_precision | 数值小数位数 (0,1,2) | 0 |
| value_unit | 数值单位后缀 | "" |
| ring_color | 进度条颜色 | #00FF00 |

### 使用示例

```conf
Widget CPU_Ring {
    class 'Ring'
    expression proc_stat::cpu('busy', 500)
    min 0
    max 100
    diameter 80
    thickness 8
    start_angle 270            # 从顶部开始顺时针
    show_value 1
    value_text_size 14
    value_precision 0
    value_unit '%'
    value_text_color 'ffffff'
    show_background 1
    background_color '333333'
    ring_color '00ff00'
    update 500
}
```

## 字体配置

### 安装中文字体

```bash
# Noto Sans CJK (推荐)
apt-get install fonts-noto-cjk

# 或者使用系统已有字体
fc-list :lang=zh
```

### 字体参数

| 参数 | 说明 | 示例 |
|------|------|------|
| Font | 字体文件路径 | `/usr/share/fonts/.../NotoSansCJK-Regular.ttc` |
| FontSize | 字号（自定义） | `8` - `64` |

## 目录结构

```
├── font_ttf.c          # TrueType 字体渲染模块
├── font_ttf.h          # 字体模块头文件
├── widget_graph.c      # Graph 组件实现
├── widget_graph.h      # Graph 组件头文件
├── widget_arc.c        # Arc 组件实现
├── widget_arc.h        # Arc 组件头文件
├── widget_ring.c       # Ring 组件实现
├── widget_ring.h       # Ring 组件头文件
├── drv_vnc.c           # VNC 驱动 (已修改支持中文)
├── drv_dpf.c           # DPF 驱动 (已修改支持中文)
├── drv_generic_graphic.c # 通用图形驱动
├── Makefile.am         # 构建配置
└── README.md           # 本文件
```

## 驱动列表

支持以下显示驱动，所有图形驱动均支持中文显示：

| 驱动 | 说明 |
|------|------|
| VNC | VNC 服务器 ✅ 中文 |
| DPF | 数码相框 ✅ 中文 |
| SamsungSPF | 三星电子相框 ✅ 中文 |
| T6963 | T6963 LCD 控制器 ✅ 中文 |
| G15 | Logitech G15 键盘 ✅ 中文 |
| X11 | X11 显示 ✅ 中文 |
| 其他 | 所有 drv_generic_graphic 驱动 ✅ 中文 |

## 常见问题

### Q: 编译报错 "freetype/freetype.h not found"

```bash
apt-get install libfreetype6-dev
```

### Q: 编译报错 "usb.h not found"

```bash
apt-get install libusb-1.0-0-dev libusb-dev
```

### Q: 中文显示乱码

检查配置文件中的 `Font` 是否使用支持中文的 TrueType 字体，如 Noto Sans CJK。

### Q: VNC 连接失败

检查端口是否被占用：
```bash
netstat -tlnp | grep 5900
```

## 更新日志

### 最新版本
- ✅ Arc 组件: 修复 reverse 方向控制 (reverse=0/1)
- ✅ Arc 组件: 修复弧线超出直径边界
- ✅ Arc 组件: 修复 value_precision 配置
- ✅ Graph 组件: 添加 value_precision 和 value_unit 参数
- ✅ Graph 组件: 修复 value_bg_color 透明背景
- ✅ 修复 Text 组件透明背景时的文字叠影问题
- ✅ 修复 VNC 颜色显示顺序

## 使用协议

基于 LCD4Linux 原协议，遵循 GPL v2。

## 参考链接

- [LCD4Linux 官网](http://lcd4linux.bulix.org/)
- [FreeType 官网](https://www.freetype.org/)
- [Noto Sans CJK 字体](https://www.google.com/get/noto/)
- [Release 下载](https://github.com/netusb/lcd4linux-display-chinese/releases)
