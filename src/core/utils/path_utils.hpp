#pragma once

#include <QFileInfo>
#include <QString>

namespace utils
{
namespace path
{

inline QString normalize_path(const QString& filepath)
{
    QFileInfo info(filepath);
    const QString canonical = info.canonicalFilePath();
    if (!canonical.isEmpty()) {
        return canonical;
    }
    return info.absoluteFilePath();
}

} // namespace path
} // namespace utils
