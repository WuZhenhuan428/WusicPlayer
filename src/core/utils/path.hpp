#pragma once

#include <QDir>
#include <QFileInfo>
#include <QString>

namespace utils
{
namespace path
{

/*================ DECLARATION  ================ */
QString canonical_path(const QString& filepath);
QString case_fold(const QString& p);
QString normalize_path(const QString& filepath);

/*================ IMPLEMENTS  ================ */
/**
 * @brief 返回规范路径, 失败则返回绝对路径
 *
 * @param filepath QString 文件路径
 * @return QString
 */
inline QString canonical_path(const QString& filepath)
{
    QFileInfo info(filepath);
    const QString canonical = info.canonicalFilePath();
    if (!canonical.isEmpty()) {
        return canonical;
    }
    return info.absoluteFilePath();
}

/**
 * @brief Windows 文件系统大小写不敏感:用于比较/去重;其他平台原样返回
 *
 * @param p QString 文件路径
 * @return QString
 */
inline QString case_fold(const QString& p)
{
#ifdef Q_OS_WIN
    return p.toCaseFolded();
#else
    return p;
#endif
}

/**
 * @brief 符号链接解析 + 分隔符统一 + 大小写折叠
 *
 * @param filepath QString 文件路径
 * @return QString
 */
inline QString normalize_path(const QString& filepath)
{
    return case_fold(QDir::fromNativeSeparators(canonical_path(filepath)));
}

} // namespace path
} // namespace utils
