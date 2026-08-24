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

/*================ DECLARATION  ================ */
QString canonical_path(const QString& filepath);
QString case_fold(const QString& p);
QString normalize_path(const QString& filepath);
bool contains_suffix(const QString& filename, const QStringList& extensions);

/// tuple definition: major, middle, minor
std::optional<std::tuple<int, int, int>> parse_versioned_unix_so(const QString& filepath);

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

/**
 * @brief 比较文件后缀, 返回查找到的位置, 支持跨平台特性
 *
 * @param suffix 目标后缀
 * @param suffixes 可用的后缀列表
 * @return size_t 目标后缀在列表中的位置
 */
inline bool contains_suffix(const QString& filename, const QStringList& extensions)
{
    QFileInfo info(filename);
    QString suffix(info.suffix());
    for (const QString& ext : extensions) {
        if (suffix.compare(ext,
#ifdef Q_OS_WIN
                           Qt::CaseInsensitive
#else
                           Qt::CaseSensitive
#endif
                           ) == 0) {
            return true;
        }
    }
    return false;
}

inline std::optional<std::tuple<int, int, int>> parse_versioned_unix_so([[maybe_unused]]const QString& filepath)
{
#if defined(Q_OS_UNIX)
    // 整体思路: 寻找 ".so" 子串, 存在时解析版本号子串是否存在 / 合法
    std::string raw = filepath.toStdString();
    if (!raw.contains(".so")) {
        return std::nullopt;
    }

    if (raw.size() < 3 || raw.ends_with(".so")) {
        return std::nullopt;
    }

    size_t pos_so = raw.rfind(".so");
    if (pos_so == std::string::npos) {
        return std::nullopt;
    }

    int major  = 0;
    int middle = 0;
    int minor  = 0;
    enum class VERSION_STATUS
    {
        START,
        MAJOR,
        POINT_1,
        MIDDLE,
        POINT_2,
        MINOR,
        ERROR,
    };
    std::string version = raw.substr(pos_so + 4, raw.size()); // +4: 跳过第一个小数点
    auto state          = VERSION_STATUS::START;
    // 循环结束后自动停止, 不存在 END 状态
    for (auto ch_ : version) {
        switch (state) {
        case VERSION_STATUS::START:
            if (ch_ >= '0' && ch_ <= '9') {
                major += (ch_ - '0');
                state = VERSION_STATUS::MAJOR;
            } else {
                state = VERSION_STATUS::ERROR;
            }
            break;
        case VERSION_STATUS::MAJOR:
            if (ch_ >= '0' && ch_ <= '9') {
                major *= 10;
                major += (ch_ - '0');
                state = VERSION_STATUS::MAJOR;
            } else if (ch_ == '.') {
                state = VERSION_STATUS::POINT_1;
            } else {
                state = VERSION_STATUS::ERROR;
            }
            break;
        case VERSION_STATUS::POINT_1:
            if (ch_ >= '0' && ch_ <= '9') {
                middle += (ch_ - '0');
                state = VERSION_STATUS::MIDDLE;
            } else {
                state = VERSION_STATUS::ERROR;
            }
            break;
        case VERSION_STATUS::MIDDLE:
            if (ch_ >= '0' && ch_ <= '9') {
                middle *= 10;
                middle += (ch_ - '0');
                state = VERSION_STATUS::MIDDLE;
            } else if (ch_ == '.') {
                state = VERSION_STATUS::POINT_2;
            } else {
                state = VERSION_STATUS::ERROR;
            }
            break;
        case VERSION_STATUS::POINT_2:
            if (ch_ >= '0' && ch_ <= '9') {
                minor += (ch_ - '0');
                state = VERSION_STATUS::MINOR;
            } else {
                state = VERSION_STATUS::ERROR;
            }
            break;
        case VERSION_STATUS::MINOR:
            if (ch_ >= '0' && ch_ <= '9') {
                minor *= 10;
                minor += (ch_ - '0');
                state = VERSION_STATUS::MINOR;
            } else {
                state = VERSION_STATUS::ERROR;
            }
            break;
        case VERSION_STATUS::ERROR:
            continue;
        }
    }
    if (state == VERSION_STATUS::MAJOR || state == VERSION_STATUS::MIDDLE ||
        state == VERSION_STATUS::MINOR) {
        return std::make_tuple(major, middle, minor);
    }
    // 只检查合法性也要按位检查, 和解析没多大区别了, 那就干脆解析吧...
    // 另外新版本的标准库已经很好用了^^:
#endif
    return std::nullopt;
}

} // namespace path
} // namespace utils
