#include "icon.hpp"

namespace utils::icon
{

QString colored_icon_path(QString name, bool is_dark)
{
    return QString(":/icons/%1/%2.svg").arg(is_dark ? "dark" : "light", name);
}

QString general_icon_path(QString name)
{
    return QString(":/icons/%1.svg").arg(name);
}

} // namespace utils::icon
