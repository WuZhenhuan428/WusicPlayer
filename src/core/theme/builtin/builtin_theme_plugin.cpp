#include "core/theme/builtin/builtin_theme_plugin.h"

#include <utility>

BuiltinThemePlugin::BuiltinThemePlugin(const QString& id, const QString& name,
                                       const QString& version, const QString& description,
                                       const QString& author, const QVector<QString>& categories,
                                       ThemePalette palette) :
    id_(id), name_(name), version_(version), description_(description), author_(author),
    categories_(categories), palette_(std::move(palette))
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

ThemePalette BuiltinThemePlugin::createPalette() const
{
    return palette_;
}
