#include "path.hpp"

#include <algorithm>
#include <ranges>

namespace utils::path
{
QString canonical_path(const QString& filepath)
{
    QFileInfo info(filepath);
    QString canonical = info.canonicalFilePath();
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
    const QString suffix(info.suffix());
    return std::ranges::any_of(extensions, [&suffix](const QString& ext) {
        return suffix.compare(ext,
#ifdef Q_OS_WIN
                              Qt::CaseInsensitive
#else
                              Qt::CaseSensitive
#endif
                              ) == 0;
    });
}

std::optional<std::tuple<int, int, int>>
parse_versioned_unix_so([[maybe_unused]] const std::string& filepath)
{
#ifdef Q_OS_UNIX
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
    enum class VersionStatus
    {
        Start,
        Major,
        Point1,
        Minor,
        Point2,
        Patch,
        Error,
    };
    std::string version = filepath.substr(pos_so + 4, filepath.size()); // +4: 跳过第一个小数点
    auto state          = VersionStatus::Start;
    // 循环结束后自动停止, 不存在 END 状态
    for (auto ch : version) {
        switch (state) {
        case VersionStatus::Start:
            if (ch >= '0' && ch <= '9') {
                major += (ch - '0');
                state = VersionStatus::Major;
            } else {
                state = VersionStatus::Error;
            }
            break;
        case VersionStatus::Major:
            if (ch >= '0' && ch <= '9') {
                major *= 10;
                major += (ch - '0');
                state = VersionStatus::Major;
            } else if (ch == '.') {
                state = VersionStatus::Point1;
            } else {
                state = VersionStatus::Error;
            }
            break;
        case VersionStatus::Point1:
            if (ch >= '0' && ch <= '9') {
                minor += (ch - '0');
                state = VersionStatus::Minor;
            } else {
                state = VersionStatus::Error;
            }
            break;
        case VersionStatus::Minor:
            if (ch >= '0' && ch <= '9') {
                minor *= 10;
                minor += (ch - '0');
                state = VersionStatus::Minor;
            } else if (ch == '.') {
                state = VersionStatus::Point2;
            } else {
                state = VersionStatus::Error;
            }
            break;
        case VersionStatus::Point2:
            if (ch >= '0' && ch <= '9') {
                patch += (ch - '0');
                state = VersionStatus::Patch;
            } else {
                state = VersionStatus::Error;
            }
            break;
        case VersionStatus::Patch:
            if (ch >= '0' && ch <= '9') {
                patch *= 10;
                patch += (ch - '0');
                state = VersionStatus::Patch;
            } else {
                state = VersionStatus::Error;
            }
            break;
        case VersionStatus::Error:
            continue;
        }
    }
    if (state == VersionStatus::Major || state == VersionStatus::Minor ||
        state == VersionStatus::Patch) {
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
