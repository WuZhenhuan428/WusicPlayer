#pragma once

#include "plugin/i_theme_plugin.h"

#include <QString>
#include <QVector>
#include <QtPlugin>

/// 主题插件模板。
/// 复制本目录后, 修改类名、元信息(id/name/version/...)、Q_PLUGIN_METADATA 的
/// json 文件名与 CMake 目标名即可。
///
/// 注意: 只继承接口链(IThemePlugin -> IBasicPlugin -> QObject), 不要直接继承
/// QObject, 否则会产生二义的 QObject 基类。
class MyThemePlugin : public IThemePlugin
{
    Q_OBJECT
    Q_INTERFACES(IBasicPlugin IThemePlugin)
    Q_PLUGIN_METADATA(IID "com.wusicplayer.IThemePlugin/1.0" FILE "my_theme_plugin.json")

public:
    // ---- IBasicPlugin ----
    [[nodiscard]] QString id() const override;
    [[nodiscard]] QString name() const override;
    [[nodiscard]] QString version() const override;
    [[nodiscard]] QString description() const override;
    [[nodiscard]] QString author() const override;
    [[nodiscard]] QVector<QString> categories() const override;

    // ---- IThemePlugin ----
    [[nodiscard]] ThemePalette create_palette() const override;
};
