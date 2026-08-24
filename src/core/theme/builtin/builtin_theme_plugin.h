#pragma once

#include "plugin/i_theme_plugin.h"

#include "core/theme/theme_palette.h"

#include <QString>
#include <QVector>

/// 内建主题插件: 把编译进程序的 ThemePalette 以 IThemePlugin 形式暴露,
/// 通过 PluginManager::register_builtin() 注册, 用于验证内建插件通路。
///
/// 注意: 只继承接口链(IThemePlugin → IBasicPlugin → QObject), 不要直接继承
/// QObject, 否则会产生二义的 QObject 基类(与标准 Qt 插件单继承模式一致)。
class BuiltinThemePlugin : public IThemePlugin
{
    Q_OBJECT
    Q_INTERFACES(IBasicPlugin IThemePlugin)

public:
    // 注意: QObject 经接口链间接继承, 不能传 parent(由组合根以 unique_ptr 持有)
    BuiltinThemePlugin(const QString& id, const QString& name, const QString& version,
                       const QString& description, const QString& author,
                       const QVector<QString>& categories, ThemePalette palette);

    // ---- IBasicPlugin ----
    QString id() const override;
    QString name() const override;
    QString version() const override;
    QString description() const override;
    QString author() const override;
    QVector<QString> categories() const override;

    // ---- IThemePlugin ----
    ThemePalette createPalette() const override;

private:
    QString id_;
    QString name_;
    QString version_;
    QString description_;
    QString author_;
    QVector<QString> categories_;
    ThemePalette palette_;
};
