#pragma once

#include "model/theme_settings_model.h"
#include <QObject>
#include <memory>

/// 主题服务——封装主题发现、应用逻辑，解耦 UI 与 ThemeManager。
class ThemeService : public QObject
{
    Q_OBJECT
public:
    explicit ThemeService(QObject* parent = nullptr);

    /// 扫描所有可用主题并更新模型
    void scanThemes();

    /// 应用模型中的第 row 行主题
    void applyTheme(int row);

    /// 获取模型（供 QTableView 绑定）
    ThemeSettingsModel* model() const;

    /// 当前主题名称
    QString currentThemeName() const;

    /// 扫描外部插件目录
    void rescanExternalPlugins(const QString& dir);

signals:
    /// 当前主题变更（无论是外部触发还是本服务内切换）
    void currentThemeChanged(const QString& name);

private:
    std::unique_ptr<ThemeSettingsModel> m_model;
};
