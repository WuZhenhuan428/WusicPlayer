# WusicPlayer 构建

## 要求 (所有平台)

- **Qt** ≥ 6.5
- **CMake** ≥ 3.24 (使用了 C++23)
- **Ninja** (推荐使用) 或 Make
- **Git** (用于获取 submodule)
- **pkg-config** (Linux / MSYS2 / Homebrew)

### 部署源代码 (git clone & git submodule)
```bash
git clone --recurse-submodules https://github.com/your/wusicplayer.git
# or if already cloned:
git submodule update --init --recursive
```

---

## Windows (MSVC)

### 1. 安装 Qt 6.5+
从 [Qt 官网](https://www.qt.io/download) 安装 MSVC 64-bit 版本.
使用了以下 Qt 组件: `Core`, `Widgets`, `Multimedia`, `Network`, `Svg`, `Sql`

### 2. 安装 FFmpeg (预编译库)
从[github](https://github.com/BtbN/FFmpeg-Builds/releases)下载 "full build shared" 版本.
解压到任意文件夹, 例如 `C:\ffmpeg`.

### 3. 安装 OpenSSL (可选)
```
winget install ShiningLight.OpenSSL
```
或者从网站下载: [slproweb.com](https://slproweb.com/products/Win32OpenSSL.html).

### 4. 编译
```powershell
# From a Developer Command Prompt for VS or after running vcvars64.bat
$env:CMAKE_PREFIX_PATH = "C:/Qt/6.x.x/msvc2022_64"
$env:FFMPEG_ROOT = "C:/ffmpeg"

cmake --preset debug -DCMAKE_PREFIX_PATH="$env:CMAKE_PREFIX_PATH" -DFFMPEG_ROOT="$env:FFMPEG_ROOT"
cmake --build --preset debug
```

### 5. 运行 (打包 DLL 动态运行库)
编译后复制以下 dll 文件到编译生成位置
- `Qt6Core.dll`, `Qt6Widgets.dll`, `Qt6Multimedia.dll`, `Qt6Svg.dll`, `Qt6Network.dll` (Qt bin/)
- `avcodec-*.dll`, `avformat-*.dll`, `avutil-*.dll`, `avfilter-*.dll` (FFmpeg bin/)
- `libcrypto-3-x64.dll`, `libssl-3-x64.dll` (OpenSSL bin/)
- 所有的 Qt `platforms/`, `styles/`, `imageformats/` 插件 DLL.

或者使用 `windeployqt`:
```powershell
windeployqt --release build/debug/WusicPlayer.exe
```

---

## Windows (MSYS2 MinGW)

### 1. 安装 MSYS2
[MSYS2 官网](https://www.msys2.org/)

### 2. 安装依赖
打开 **MSYS2 MINGW64** 终端:
```bash
pacman -Syu
pacman -S \
    mingw-w64-x86_64-qt6-base \
    mingw-w64-x86_64-qt6-multimedia \
    mingw-w64-x86_64-qt6-svg \
    mingw-w64-x86_64-qt6-sql \
    mingw-w64-x86_64-ffmpeg \
    mingw-w64-x86_64-taglib \
    mingw-w64-x86_64-openssl \
    mingw-w64-x86_64-zlib \
    mingw-w64-x86_64-ninja \
    mingw-w64-x86_64-gcc \
    mingw-w64-x86_64-cmake \
    mingw-w64-x86_64-pkgconf
```

### 3. 编译
```bash
cmake --preset debug
cmake --build --preset debug
```

---

## Linux (Debian/Ubuntu)

### 1. 安装依赖
```bash
sudo apt install qt6-base-dev qt6-multimedia-dev qt6-svg-dev \
    libavcodec-dev libavformat-dev libavutil-dev libavfilter-dev \
    libtag1-dev libssl-dev zlib1g-dev \
    ninja-build pkgconf git
```

### 2. 编译
```bash
cmake --preset debug
cmake --build --preset debug
```

---

## macOS

### 1. 安装依赖
```bash
brew install qt@6 ffmpeg taglib openssl zlib ninja pkgconf
```

### 2. 编译
```bash
cmake --preset debug \
    -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)" \
    -DFFMPEG_ROOT="$(brew --prefix ffmpeg)"
cmake --build --preset debug
```

---

## CMake 选项

| 选项                 | 默认    | 描述                                   |
|----------------------|---------|----------------------------------------|
| `WUSIC_BUILD_TESTS`  | `ON`    | 编译单元测试                           |
| `WUSIC_ENABLE_GPROF` | `OFF`   | 启用 gprof 性能分析 (GCC only)         |
| `CMAKE_PREFIX_PATH`  | —       | Qt 安装路径                            |
| `FFMPEG_ROOT`        | —       | FFmpeg 安装路径 (主要用于 Windows)     |
| `CMAKE_BUILD_TYPE`   | —       | `Debug` / `Release` / `RelWithDebInfo` |
