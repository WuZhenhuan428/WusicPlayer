# WusicPlayer 插件模板

本目录提供开发 WusicPlayer 外部插件的**样板模板**与**规范说明**。
所有外部插件均为动态库(`.so`/`.dll`), 运行时由 `PluginManager` 通过 `QPluginLoader` 加载。

## 目录结构

- `theme_plugin/` — 主题插件模板(实现 `IThemePlugin`)
- `eq_plugin/` — EQ 插件模板(实现 `IEqPlugin`)

复制对应子目录即可开始新插件, 记得同时修改类名、`Q_PLUGIN_METADATA` 中的 json 文件名、`id()` 与 CMake 目标名。

## 插件如何被加载

1. 插件 `.so` 放入应用可执行文件旁的 `plugins/` 目录(支持递归子目录);
2. `PluginManager::scan_directory()` 扫描目录, 对 `.so` 调用 `load_plugin()`;
3. `QPluginLoader` 读取动态库内嵌元数据(`Q_PLUGIN_METADATA` 声明的 IID 与 json);
4. `qobject_cast<IBasicPlugin*>` 校验基础接口, 注册进管理器;
5. 主题: `ThemeManager::external_themes()` 列出, 可在设置中按名称切换;
   EQ: `EQWidget` 通过 `plugin_manager()->plugins<IEqPlugin>()` 枚举显示在面板下拉框。

## 样板代码要求(必须满足)

### 1. 类只继承接口链

```cpp
class MyPlugin : public IThemePlugin { ... };            // ✅ 正确
class MyPlugin : public QObject, public IThemePlugin {}; // ❌ QObject 二义, 编译报错
```

插件类**不要直接继承 QObject**。`QObject` 经接口链
`IXxxPlugin → IBasicPlugin → QObject` 间接继承(与标准 Qt 插件单继承模式一致)。

### 2. 声明 Q_OBJECT 与 Q_INTERFACES

```cpp
Q_OBJECT
Q_INTERFACES(IBasicPlugin IThemePlugin)   // 双向 qobject_cast 生效
```

### 3. 内嵌插件元数据

```cpp
Q_PLUGIN_METADATA(IID "com.wusicplayer.IThemePlugin/1.0" FILE "my_theme_plugin.json")
```

- `IID` 必须与 `Q_DECLARE_INTERFACE` 中声明的**完全一致**;
- `FILE` 指定的 json 会被内嵌进动态库, 供 `resolve_descriptor()` **优先读取**
  (`id/name/version/description/author/categories`); json 缺失字段时回退到调用插件接口函数。

### 4. 版本号约定

- 接口 IID 中的版本号统一为 `1.0`(内部修改**不增加**版本号);
- `version()` 返回 `"1.0"`;
- `id()` 命名: `<域名>.<作者/组织>.<插件名称>`, 如 `com.wusicplayer.theme.green`(不带版本号后缀)。

### 5. 编译要求

- `CMAKE_AUTOMOC ON`(头文件含 `Q_OBJECT`, 必须由 moc 处理);
- 构建为 `MODULE` 库(非 `SHARED`), 去掉 `lib` 前缀(`PREFIX ""`);
- 链接 `Qt6::Core`(必需)与 `Qt6::Widgets`(EQ 插件返回 QWidget 必需);
- include 路径指向 WusicPlayer 项目的 `include/`(接口头)与 `src/`(如 `theme_palette.h`)。

### 6. 宿主要求(重要)

- 宿主 WusicPlayer 必须以 `-rdynamic` 链接(见 `WusicPlayer/src/CMakeLists.txt`):
  接口的 moc 符号(如 `IBasicPlugin::staticMetaObject`)由静态库链接进**可执行文件**,
  插件 `dlopen` 加载时必须能解析这些符号, 而可执行文件默认不导出符号;
  缺少时插件加载报 `undefined symbol: IBasicPlugin::staticMetaObject`。
- 部署: 编译产物(`.so`)复制到应用可执行文件旁的 `plugins/` 目录, 重启应用即被加载。
  外部插件路径会在 `WusicPlayer.json` 中持久化, 下次启动自动恢复。

## 接口速览

| 接口 | 头文件 | 必须实现 |
|------|--------|----------|
| `IBasicPlugin` | `plugin/i_basic_plugin.h` | `id/name/version/description/author/categories` |
| `IThemePlugin` | `plugin/i_theme_plugin.h` | `createPalette()` 返回完整 `ThemePalette` |
| `IEqPlugin` | `plugin/i_eq_plugin.h` | `create_eq_widget/eq_config/apply_current/revert/reset/restore_from_config` + 信号 `sgn_config_changed` |

> `ThemePalette` 结构见 `src/core/theme/theme_palette.h`(QPalette 角色 + 语义颜色 + 尺寸)。
> 参考调色板:`src/core/theme/builtin/wusic_dark_palette.h`。
