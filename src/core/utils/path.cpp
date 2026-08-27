#include "path.hpp"

namespace utils::path
{
QString canonical_path(const QString& filepath)
{
    QFileInfo info(filepath);
    const QString canonical = info.canonicalFilePath();
    if (!canonical.isEmpty()) {
        return canonical;
    }
    return info.absoluteFilePath();
}

QString case_fold(const QString& p)
{
#ifdef Q_OS_WIN
    return p.toCaseFolded();
#else
    return p;
#endif
}

QString normalize_path(const QString& filepath)
{
    return case_fold(QDir::fromNativeSeparators(canonical_path(filepath)));
}

bool contains_suffix(const QString& filename, const QStringList& extensions)
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

std::optional<std::tuple<int, int, int>>
parse_versioned_unix_so([[maybe_unused]] const std::string& filepath)
{
#if defined(Q_OS_UNIX)
    // 整体思路: 寻找 ".so" 子串, 存在时解析版本号子串是否存在 / 合法
    if (!filepath.contains(".so")) {
        return std::nullopt;
    }

    if (filepath.size() < 3 || filepath.ends_with(".so")) {
        return std::nullopt;
    }

    size_t pos_so = filepath.rfind(".so");
    if (pos_so == std::string::npos) {
        return std::nullopt;
    }

    int major = 0;
    int minor = 0;
    int patch = 0;
    enum class VERSION_STATUS
    {
        START,
        MAJOR,
        POINT_1,
        MINOR,
        POINT_2,
        PATCH,
        ERROR,
    };
    std::string version = filepath.substr(pos_so + 4, filepath.size()); // +4: 跳过第一个小数点
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
            } else if (ch_ == '.') {
                state = VERSION_STATUS::POINT_2;
            } else {
                state = VERSION_STATUS::ERROR;
            }
            break;
        case VERSION_STATUS::POINT_2:
            if (ch_ >= '0' && ch_ <= '9') {
                patch += (ch_ - '0');
                state = VERSION_STATUS::PATCH;
            } else {
                state = VERSION_STATUS::ERROR;
            }
            break;
        case VERSION_STATUS::PATCH:
            if (ch_ >= '0' && ch_ <= '9') {
                patch *= 10;
                patch += (ch_ - '0');
                state = VERSION_STATUS::PATCH;
            } else {
                state = VERSION_STATUS::ERROR;
            }
            break;
        case VERSION_STATUS::ERROR:
            continue;
        }
    }
    if (state == VERSION_STATUS::MAJOR || state == VERSION_STATUS::MINOR ||
        state == VERSION_STATUS::PATCH) {
        return std::make_tuple(major, minor, patch);
    }
    // 只检查合法性也要按位检查, 和解析没多大区别了, 那就干脆解析吧...
    // 另外新版本的标准库已经很好用了^^:
#endif
    return std::nullopt;
}

std::optional<std::tuple<int, int, int>>
parse_versioned_unix_so([[maybe_unused]] const QString& filepath)
{
    return parse_versioned_unix_so(filepath.toStdString());
}
} // namespace utils::path
