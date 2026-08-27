#pragma once

#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QStringList>

#include <optional>
#include <string>
#include <tuple>

namespace utils
{
namespace path
{

/// 返回规范路径, 失败则返回绝对路径
QString canonical_path(const QString& filepath);
/// Windows 文件系统大小写不敏感:用于比较/去重;其他平台原样返回
QString case_fold(const QString& p);
/// 符号链接解析 + 分隔符统一 + 大小写折叠
QString normalize_path(const QString& filepath);
/// 比较文件后缀, 适配特定平台大小写(不)敏感
bool contains_suffix(const QString& filename, const QStringList& extensions);

/// tuple definition: major, minor, patch
std::optional<std::tuple<int, int, int>> parse_versioned_unix_so(const std::string& filepath);
std::optional<std::tuple<int, int, int>> parse_versioned_unix_so(const QString& filepath);

} // namespace path
} // namespace utils
