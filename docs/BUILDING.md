# Building WusicPlayer

## Prerequisites (All Platforms)

- **CMake** ≥ 3.21
- **Ninja** (recommended) or Make
- **Git** (for submodules)
- **pkg-config** (Linux / MSYS2 / Homebrew)

### Clone & submodules
```bash
git clone --recurse-submodules https://github.com/your/wusicplayer.git
# or if already cloned:
git submodule update --init --recursive
```

---

## Windows (MSVC)

### 1. Install Qt 6.5+
Download from https://www.qt.io/download and install the MSVC 64-bit version.
Make sure to select `Qt Multimedia` and `Qt SVG` modules.

### 2. Install FFmpeg (pre-built shared libs)
Download "full build shared" from https://github.com/BtbN/FFmpeg-Builds/releases.
Extract to e.g. `C:\ffmpeg`.

### 3. Install OpenSSL (optional, for network features)
```
winget install ShiningLight.OpenSSL
```
Or download from https://slproweb.com/products/Win32OpenSSL.html.

### 4. Build
```powershell
# From a Developer Command Prompt for VS or after running vcvars64.bat
$env:CMAKE_PREFIX_PATH = "C:/Qt/6.x.x/msvc2022_64"
$env:FFMPEG_ROOT = "C:/ffmpeg"

cmake --preset debug -DCMAKE_PREFIX_PATH="$env:CMAKE_PREFIX_PATH" -DFFMPEG_ROOT="$env:FFMPEG_ROOT"
cmake --build --preset debug
```

### 5. Run (bundle DLLs)
After building, copy the following DLLs next to the executable:
- `Qt6Core.dll`, `Qt6Widgets.dll`, `Qt6Multimedia.dll`, `Qt6Svg.dll`, `Qt6Network.dll` (from Qt bin/)
- `avcodec-*.dll`, `avformat-*.dll`, `avutil-*.dll`, `avfilter-*.dll` (from FFmpeg bin/)
- `libcrypto-3-x64.dll`, `libssl-3-x64.dll` (from OpenSSL bin/)
- All Qt `platforms/`, `styles/`, `imageformats/` plugin DLLs

Or use `windeployqt`:
```powershell
windeployqt --release build/debug/WusicPlayer.exe
```

---

## Windows (MSYS2 MinGW)

### 1. Install MSYS2
Download from https://www.msys2.org/.

### 2. Install dependencies
Open **MSYS2 MINGW64** terminal:
```bash
pacman -Syu
pacman -S \
    mingw-w64-x86_64-qt6-base \
    mingw-w64-x86_64-qt6-multimedia \
    mingw-w64-x86_64-qt6-svg \
    mingw-w64-x86_64-ffmpeg \
    mingw-w64-x86_64-taglib \
    mingw-w64-x86_64-openssl \
    mingw-w64-x86_64-zlib \
    mingw-w64-x86_64-ninja \
    mingw-w64-x86_64-gcc \
    mingw-w64-x86_64-cmake \
    mingw-w64-x86_64-pkgconf
```

### 3. Build
```bash
cmake --preset debug
cmake --build --preset debug
```

---

## Linux (Debian/Ubuntu)

### 1. Install dependencies
```bash
sudo apt install \
    qt6-base-dev qt6-multimedia-dev qt6-svg-dev \
    libavcodec-dev libavformat-dev libavutil-dev libavfilter-dev \
    libtag1-dev libssl-dev zlib1g-dev \
    ninja-build pkgconf git
```

### 2. Build
```bash
cmake --preset debug
cmake --build --preset debug
```

---

## macOS

### 1. Install dependencies
```bash
brew install qt@6 ffmpeg taglib openssl zlib ninja pkgconf
```

### 2. Build
```bash
cmake --preset debug \
    -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)" \
    -DFFMPEG_ROOT="$(brew --prefix ffmpeg)"
cmake --build --preset debug
```

---

## CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `WUSIC_BUILD_TESTS` | `ON` | Build unit tests |
| `WUSIC_ENABLE_GPROF` | `OFF` | Enable gprof profiling (GCC only) |
| `CMAKE_PREFIX_PATH` | — | Qt installation prefix |
| `FFMPEG_ROOT` | — | FFmpeg installation prefix (mainly for Windows) |
| `CMAKE_BUILD_TYPE` | — | `Debug` / `Release` / `RelWithDebInfo` |
