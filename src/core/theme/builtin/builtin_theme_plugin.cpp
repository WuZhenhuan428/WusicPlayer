#include "core/theme/builtin/builtin_theme_plugin.h"

#include <utility>

BuiltinThemePlugin::BuiltinThemePlugin(QString id, QString name, QString version,
                                       QString description, QString author,
                                       const QVector<QString>& categories, ThemePalette palette) :
    id_(std::move(id)), name_(std::move(name)), version_(std::move(version)),
    description_(std::move(description)), author_(std::move(author)), categories_(categories),
    palette_(std::move(palette))
{}

QString BuiltinThemePlugin::id() const
{
    return id_;
}

QString BuiltinThemePlugin::name() const
{
    return name_;
}

QString BuiltinThemePlugin::version() const
{
    return version_;
}

QString BuiltinThemePlugin::description() const
{
    return description_;
}

QString BuiltinThemePlugin::author() const
{
    return author_;
}

QVector<QString> BuiltinThemePlugin::categories() const
{
    return categories_;
}

ThemePalette BuiltinThemePlugin::create_palette() const
{
    return palette_;
}
