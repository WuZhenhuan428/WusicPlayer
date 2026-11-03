#pragma once

#include "core/config_manager/i_configurable.h"
#include "core/theme/theme_palette.h"
#include <QMap>
#include <QObject>

class QStyle;
class PluginManager;

/// 主题管理器——单例，负责主题发现、切换和持久化。
/// 支持三种来源：系统主题（QStyleFactory）、内置主题（编译进程序的调色板）、外部插件（.so/.dll）。
class ThemeManager : public QObject, public IConfigurable
{
    Q_OBJECT
public:
    enum class Source
    {
        System,
        Builtin,
        External,
    };

    static ThemeManager& instance();

    // ---- 扫描外部主题插件 ----
    void scan_external_plugins(const QString& dir);

    /// 注入 PluginManager(组合根调用): 内建/外部主题插件统一由 PluginManager 登记,
    /// 本管理器只负责查询与应用。
    void set_plugin_manager(PluginManager* plugin_manager);

    // ---- 查询可用主题列表 ----
    [[nodiscard]] QStringList system_themes() const;   // QStyleFactory::keys()
    [[nodiscard]] QStringList builtin_themes() const;  // 内建主题插件名称(register_builtin)
    [[nodiscard]] QStringList external_themes() const; // 外部主题插件名称(.so 加载)

    // ---- 主题元信息(来自插件描述/调色板) ----
    [[nodiscard]] QString
    theme_author(const QString& name) const;                     // 插件 author(无则 "WusicPlayer")
    [[nodiscard]] bool theme_is_dark(const QString& name) const; // 插件调色板 isDark

    // ---- 应用主题 ----
    void apply_system_theme(const QString& key);    // "Fusion", "Windows" 等
    void apply_builtin_theme(const QString& name);  // "Wusic Dark", "Wusic Light"
    void apply_external_theme(const QString& name); // 按插件名称

    // ---- 运行状态 ----
    [[nodiscard]] Source current_source() const
    {
        return current_source_;
    }
    [[nodiscard]] QString current_theme_name() const
    {
        return currentName_;
    }
    [[nodiscard]] const ThemePalette*
    current_palette() const; // 仅 Builtin/External 有效，System 返回 nullptr
    [[nodiscard]] const ThemePalette*
    palette_by_name(const QString& name) const; // 按名称查询内置/外部调色板
    [[nodiscard]] QStyle* current_style() const
    {
        return current_style_;
    }

    // ---- 图标模式（独立于主题的明/暗选择） ----
    enum class IconMode
    {
        IconAuto,
        IconLight,
        IconDark,
    };
    [[nodiscard]] IconMode icon_mode() const
    {
        return icon_mode_;
    }
    void set_icon_mode(IconMode mode);
    [[nodiscard]] bool effective_icon_is_dark() const; // 根据当前主题+图标模式计算实际图标明暗

    // ---- IConfigurable ----
    void load_from_json(const QJsonObject& json) override;
    QJsonObject save_to_json() override;
    [[nodiscard]] QString config_sub_key() const override
    {
        return QStringLiteral("theme");
    }

signals:
    void sgn_theme_changed();

private:
    ThemeManager();
    void set_current(Source source, const QString& name, QStyle* style);

    // 内置调色板注册表: name → ThemePalette
    QMap<QString, ThemePalette> builtin_palettes_;
    // 非拥有: 组合根注入的 PluginManager(主题插件统一登记处)
    PluginManager* plugin_manager_ = nullptr;

    Source current_source_         = Source::System;
    QString currentName_           = QStringLiteral("Fusion");
    QStyle* current_style_         = nullptr;            // QApplication 拥有所有权（setStyle 后）
    IconMode icon_mode_            = IconMode::IconAuto; // 默认跟随主题
};
