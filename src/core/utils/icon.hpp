#pragma once

#include <QString>

namespace utils::icon
{

QString colored_icon_path(const QString& name, bool is_dark);
QString general_icon_path(const QString& name);

} // namespace utils::icon
