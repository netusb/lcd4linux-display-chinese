# LCD4Linux 中文显示支持

为 LCD4Linux 添加 TrueType/FreeType 中文字体渲染支持。

## 功能特性

- ✅ TrueType 中文字体渲染支持
- ✅ VNC 驱动中文支持
- ✅ DPF (数码相框) 驱动中文支持
- ✅ SamsungSPF 驱动支持
- ✅ 所有驱动已编译
- ✅ 支持 UTF-8 中文显示

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
    FontSize     '16'
    Port         '5900'
    Xres         '320'
    Yres         '240'
    Bpp          '4'
    Password     '123456'
}

Widget Time {
    class 'Text'
    expression strftime('%H:%M:%S', time())
    width 8
    speed 1000
}

Widget Test {
    class 'Text'
    expression '测试中文字符'
    width 20
}

Layout Main {
    Row1 { Col1 'Time' }
    Row2 { Col1 'Test' }
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
    FontSize     '24'
}

Widget Test {
    class 'Text'
    expression '中文测试'
    width 30
}

Layout Main {
    Row1 { Col1 'Test' }
}

Display 'DPF'
Layout 'Main'
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
| FontSize | 字号 | `16`, `18`, `24` |

## 目录结构

```
├── font_ttf.c          # TrueType 字体渲染模块
├── font_ttf.h          # 字体模块头文件
├── drv_vnc.c           # VNC 驱动 (已修改支持中文)
├── drv_dpf.c           # DPF 驱动 (已修改支持中文)
├── drv_generic_graphic.c # 通用图形驱动
├── Makefile.am         # 构建配置
└── README.md           # 本文件
```

## 驱动列表

支持以下显示驱动：

| 驱动 | 说明 |
|------|------|
| VNC | VNC 服务器，支持中文 |
| DPF | 数码相框，支持中文 |
| SamsungSPF | 三星电子相框 |
| X11 | X11 显示 |
| HD44780 | 字符 LCD |
| MatrixOrbital | Matrix Orbital LCD |
| Crystalfontz | Crystalfontz LCD |
| ... | 更多驱动 |

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
