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

### VNC 显示配置

```conf
Display VNC {
    Driver       'VNC'
    Font         '/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc'
    FontSize     '16'        # 可选: 8-64
    Port         '5900'
    Xres         '320'
    Yres         '240'
    Bpp          '4'
}

Widget Test {
    class 'Text'
    expression '测试中文字符'
    width 20
}

Layout Main {
    Row1 { Col1 'Test' }
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
| style | 样式 (0=线, 1=面积) | 0 |
| color | 线条颜色 | #00FF00 |
| fill | 填充颜色 | #008000 |
| bg | 背景颜色 | #000000 |
| grid | 网格颜色 | #404040 |

### 使用示例

```conf
Widget CPU_Graph {
    class 'graph'
    expression 'cpu::cpu0'
    width 100
    height 40
    min 0
    max 100
    update 500
    points 60
    style 1              # 1=面积填充, 0=纯线条
    color '#00FF00'      # 绿色线条
    fill '#003300'       # 半透明绿色填充
}

Widget Memory_Graph {
    class 'graph'
    expression 'mem::used'
    width 100
    height 40
    min 0
    max 'mem::total'
    update 1000
    points 50
}

Layout Main {
    Row1 { Col1 'CPU_Graph' }
    Row2 { Col1 'Memory_Graph' }
}
```

## Arc 组件 (弧形仪表盘)

Arc 组件用于显示单个数值的仪表盘，类似于汽车速度表或 AIDA64 的传感器显示。

### 配置参数

| 参数 | 说明 | 默认值 |
|------|------|--------|
| expression | 数据表达式 | 必需 |
| min | 最小值 | 0 |
| max | 最大值 | 100 |
| update | 更新间隔(毫秒) | 1000 |
| style | 样式 (semi/quarter/full) | semi |
| ticks | 主刻度数量 | 5 |
| minor | 主刻度间小刻度数量 | 5 |
| thickness | 弧线厚度 | 8 |
| arc | 弧线背景颜色 | #404040 |
| needle | 指针颜色 | #FF0000 |
| center | 中心圆颜色 | #808080 |
| bg | 背景颜色 | #000000 |

### 使用示例

```conf
Widget CPU_Arc {
    class 'arc'
    expression 'cpu::used'
    width 80
    height 50
    min 0
    max 100
    update 1000
    style 'semi'         # semi=半圆, quarter=四分之一, full=整圆
    ticks 5
    minor 5
    thickness 8
    needle '#FF0000'     # 红色指针
    arc '#404040'        # 灰色弧线背景
}

Widget Temperature_Arc {
    class 'arc'
    expression 'thermal::thermal_zone0'
    width 60
    height 40
    min 30
    max 90
    update 2000
    style 'semi'
    ticks 6
    thickness 6
}

Layout Main {
    Row1 { Col1 'CPU_Arc' Col2 'Temperature_Arc' }
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

## 使用协议

基于 LCD4Linux 原协议，遵循 GPL v2。

## 参考链接

- [LCD4Linux 官网](http://lcd4linux.bulix.org/)
- [FreeType 官网](https://www.freetype.org/)
- [Noto Sans CJK 字体](https://www.google.com/get/noto/)
