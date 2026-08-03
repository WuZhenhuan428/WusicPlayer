#pragma once

#include "core/theme/theme_palette.h"
#include "core/config_manager/i_configurable.h"
#include <QMap>
#include <QObject>

class QStyle;

/// 主题管理器——单例，负责主题发现、切换和持久化。
/// 支持三种来源：系统主题（QStyleFactory）、内置主题（编译进程序的调色板）、外部插件（.so/.dll）。
class ThemeManager : public QObject, public IConfigurable
{
    Q_OBJECT
public:
    enum Source
    {
        System,
        Builtin,
        External
    };

    static ThemeManager& instance();

    // ---- 注册内置调色板（main.cpp 启动时调用） ----
    void register_builtin_palette(const ThemePalette& palette);

    // ---- 扫描外部主题插件 ----
    void scan_external_plugins(const QString& dir);

    // ---- 查询可用主题列表 ----
    QStringList system_themes() const;   // QStyleFactory::keys()
    QStringList builtin_themes() const;  // 已注册的内置调色板名称
    QStringList external_themes() const; // 已发现的外部插件名称

    // ---- 应用主题 ----
    void apply_system_theme(const QString& key);    // "Fusion", "Windows" 等
    void apply_builtin_theme(const QString& name);  // "Wusic Dark", "Wusic Light"
    void apply_external_theme(const QString& name); // 按插件名称

    // ---- 运行状态 ----
    Source current_source() const
    {
        return m_current_source;
    }
    QString current_theme_name() const
    {
        return m_currentName;
    }
    const ThemePalette* current_palette() const; // 仅 Builtin/External 有效，System 返回 nullptr
    const ThemePalette* palette_by_name(const QString& name) const; // 按名称查询内置/外部调色板
    QStyle* current_style() const
    {
        return m_current_style;
    }

    // ---- 图标模式（独立于主题的明/暗选择） ----
    enum IconMode
    {
        IconAuto,
        IconLight,
        IconDark
    };
    IconMode icon_mode() const
    {
        return m_icon_mode;
    }
    void set_icon_mode(IconMode mode);
    bool effective_icon_is_dark() const; // 根据当前主题+图标模式计算实际图标明暗

    // ---- IConfigurable ----
    void load_from_json(const QJsonObject& json) override;
    QJsonObject save_to_json() override;
    QString config_sub_key() const override
    {
        return QStringLiteral("theme");
    }

signals:
    void sgn_theme_changed();

private:
    ThemeManager();
    void set_current(Source source, const QString& name, QStyle* style);

    // 内置调色板注册表: name → ThemePalette
    QMap<QString, ThemePalette> m_builtinPalettes;
    // 外部插件注册表: name → filePath
    QMap<QString, QString> m_externalPlugins;

    Source m_current_source = System;
    QString m_currentName  = QStringLiteral("Fusion");
    QStyle* m_current_style = nullptr;  // QApplication 拥有所有权（setStyle 后）
    IconMode m_icon_mode    = IconAuto; // 默认跟随主题
};
