#pragma once

#include <QString>
#include <QStringList>

namespace utils::string
{

inline QString unfold_string(const QStringList& list, char separator)
{
    if (list.size() == 0) {
        return {};
    }
    QString str = list[0];
    for (ssize_t i = 1; i < list.size(); ++i) {
        str += separator;
        str += list[i];
    }
    return str;
}

} // namespace utils::string
