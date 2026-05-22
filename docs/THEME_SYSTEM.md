# WusicPlayer 主题系统

基于 Qt QStyle（非 QSS）的主题管理器，支持系统主题、内置主题和外部插件主题的运行时切换。

## 架构概览

```
src/core/theme/
├── ThemePalette.h              # 调色板数据结构——整个系统的唯一数据源
├── WusicProxyStyle.h/cpp       # QProxyStyle 子类，读取 ThemePalette 绘制控件
├── ThemeManager.h/cpp          # 单例管理器，主题发现、切换、持久化
├── builtin/
│   ├── WusicDarkPalette.h      # 内置暗色调色板
│   └── WusicLightPalette.h     # 内置亮色调色板
└── plugin/
    └── IThemePlugin.h          # 外部主题插件接口
```

### 数据流

```
ThemePalette ──→ WusicProxyStyle ──→ QApplication::setStyle()
     ↑                  ↑
     │                  │
内置调色板          外部插件 (.so)
(编译进程序)        (运行时加载)
     │                  │
     └──── ThemeManager ─┘
              │
              ├── ConfigManager (持久化到 WusicPlayer.json)
              └── 信号 themeChanged() (通知 UI 刷新)
```

## 三种主题来源

### 1. 系统主题

直接使用 `QStyleFactory` 提供的平台样式：

```cpp
ThemeManager::instance().applySystemTheme("Fusion");   // 跨平台一致
ThemeManager::instance().applySystemTheme("Windows");  // Windows 原生
```

可用键名通过 `QStyleFactory::keys()` 查询，或调用 `ThemeManager::systemThemes()`。

### 2. 内置主题

编译进程序的 `WusicProxyStyle`，数据来自 `ThemePalette`。当前内置：

| 名称 | 文件 |
|------|------|
| Wusic Dark | `builtin/WusicDarkPalette.h` |
| Wusic Light | `builtin/WusicLightPalette.h` |

在 `main.cpp` 中注册：

```cpp
themeMgr.registerBuiltinPalette(darkPalette());
themeMgr.registerBuiltinPalette(lightPalette());
```

应用：

```cpp
ThemeManager::instance().applyBuiltinTheme("Wusic Dark");
```

### 3. 外部插件主题

实现 `IThemePlugin` 接口的 `.so`/`.dll`，放到 `plugins/themes/` 目录。

```cpp
// 示例：实现一个外部主题插件
class MyTheme : public QObject, public IThemePlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.wusicplayer.IThemePlugin/1.0")
    Q_INTERFACES(IThemePlugin)
public:
    QString name() const override { return "My Theme"; }
    ThemePalette createPalette() const override {
        // 填充 ThemePalette 并返回
    }
};
```

扫描与加载：

```cpp
ThemeManager::instance().scanExternalPlugins("/path/to/plugins/themes");
ThemeManager::instance().applyExternalTheme("My Theme");
```

## ThemePalette 字段说明

`ThemePalette` 包含三类字段：

### QPalette 角色（自动映射到标准色角色）

`window`, `windowText`, `base`, `alternateBase`, `text`, `button`, `buttonText`, `brightText`, `highlight`, `highlightedText`, `toolTipBase`, `toolTipText`

### 语义颜色（供 WusicProxyStyle 绘制特定 UI 区域）

`sidebarBg`, `controlBarBg`, `controlBarBorder`, `menuBarBg`, `splitterHandle`, `scrollbarBg`, `scrollbarHandle`, `progressBarBg`, `progressBarFill`, `itemHover`, `itemSelected`, `frameBorder`

### 尺寸参数（QStyle::pixelMetric）

`buttonRadius`, `panelRadius`, `menuRadius`, `scrollbarWidth`, `sliderGrooveH`, `sliderHandleW`, `separatorW`

## WusicProxyStyle 覆写清单

| 虚函数 | 作用 |
|--------|------|
| `standardPalette()` | 返回自定义 QPalette |
| `pixelMetric()` | 滚动条宽度、滑块尺寸等 |
| `drawControl(CE_PushButton)` | 圆角按钮 |
| `drawControl(CE_MenuBarEmptyArea)` | 菜单栏背景 |
| `drawPrimitive(PE_PanelItemViewItem)` | 列表项 hover/选中背景 |
| `drawPrimitive(PE_Frame)` | 框架边框 |
| `drawComplexControl(CC_Slider)` | 自定义滑块（圆形手柄） |
| `polish()` | 自动开启 `QTreeView::alternatingRowColors` |

未覆写的绘制方法全部委托给基础样式（Fusion）。

## 运行时切换

```cpp
// 切换主题——所有控件自动 repolish
ThemeManager::instance().applyBuiltinTheme("Wusic Light");

// 主题变更时会发射信号
connect(&ThemeManager::instance(), &ThemeManager::themeChanged, [] {
    // 如有需要，在此刷新自定义绘制
});
```

`QApplication::setStyle()` 会自动：
- 对每个现有控件调用 `unpolish()`（旧样式）和 `polish()`（新样式）
- 触发全部控件重绘

## 图标主题适配

`WControlBar` 使用 `iconPath()` 辅助函数根据当前主题的 `isDark` 字段自动选择：

```cpp
":/icons/light/play.svg"   // isDark == false
":/icons/dark/play.svg"    // isDark == true
```

添加新图标时，需在 `src/static/icons/light/` 和 `src/static/icons/dark/` 两个目录下各放一份，
并在 `src/static/CMakeLists.txt` 的 `qt_add_resources` 中注册。

## 持久化

`ThemeManager` 实现 `IConfigurable`，主题选择保存在 `WusicPlayer.json` 中：

```json
{
  "theme": {
    "source": "builtin",
    "name": "Wusic Dark"
  }
}
```

下次启动时由 `ConfigManager::loadAll()` 自动恢复。

## 新增内置主题

1. 参考 `builtin/WusicDarkPalette.h` 创建新的调色板头文件
2. 在 `main.cpp` 中调用 `themeMgr.registerBuiltinPalette(yourPalette())`
3. 完成
