#pragma once

#include "ThemePalette.h"
#include "core/ConfigManager/IConfigurable.h"
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
    void registerBuiltinPalette(const ThemePalette& palette);

    // ---- 扫描外部主题插件 ----
    void scanExternalPlugins(const QString& dir);

    // ---- 查询可用主题列表 ----
    QStringList systemThemes() const;   // QStyleFactory::keys()
    QStringList builtinThemes() const;  // 已注册的内置调色板名称
    QStringList externalThemes() const; // 已发现的外部插件名称

    // ---- 应用主题 ----
    void applySystemTheme(const QString& key);    // "Fusion", "Windows" 等
    void applyBuiltinTheme(const QString& name);  // "Wusic Dark", "Wusic Light"
    void applyExternalTheme(const QString& name); // 按插件名称

    // ---- 运行状态 ----
    Source currentSource() const
    {
        return m_currentSource;
    }
    QString currentThemeName() const
    {
        return m_currentName;
    }
    const ThemePalette* currentPalette() const; // 仅 Builtin/External 有效，System 返回 nullptr
    const ThemePalette* paletteByName(const QString& name) const; // 按名称查询内置/外部调色板
    QStyle* currentStyle() const
    {
        return m_currentStyle;
    }

    // ---- 图标模式（独立于主题的明/暗选择） ----
    enum IconMode
    {
        IconAuto,
        IconLight,
        IconDark
    };
    IconMode iconMode() const
    {
        return m_iconMode;
    }
    void setIconMode(IconMode mode);
    bool effectiveIconIsDark() const; // 根据当前主题+图标模式计算实际图标明暗

    // ---- IConfigurable ----
    void loadFromJson(const QJsonObject& json) override;
    QJsonObject saveToJson() override;
    QString configSubKey() const override
    {
        return QStringLiteral("theme");
    }

signals:
    void themeChanged();

private:
    ThemeManager();
    void setCurrent(Source source, const QString& name, QStyle* style);

    // 内置调色板注册表: name → ThemePalette
    QMap<QString, ThemePalette> m_builtinPalettes;
    // 外部插件注册表: name → filePath
    QMap<QString, QString> m_externalPlugins;

    Source m_currentSource = System;
    QString m_currentName  = QStringLiteral("Fusion");
    QStyle* m_currentStyle = nullptr;  // QApplication 拥有所有权（setStyle 后）
    IconMode m_iconMode    = IconAuto; // 默认跟随主题
};
