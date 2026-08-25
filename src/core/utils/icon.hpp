#pragma once

#include <QString>

namespace utils
{
namespace icon
{

inline QString colored_icon_path(QString name, bool is_dark)
{
    return QString(":/icons/%1/%2.svg").arg(is_dark ? "dark" : "light", name);
}

inline QString general_icon_path(QString name)
{
    return QString(":/icons/%1.svg").arg(name);
}

} // namespace icon
} // namespace utils
