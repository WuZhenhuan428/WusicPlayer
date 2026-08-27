#pragma once

#include "core/types.h"

#include <taglib/asffile.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/fileref.h>
#include <taglib/flacfile.h>
#include <taglib/id3v2frame.h>
#include <taglib/id3v2header.h>
#include <taglib/id3v2tag.h>
#include <taglib/mp4file.h>
#include <taglib/mpegfile.h>
#include <taglib/oggflacfile.h>
#include <taglib/opusfile.h>
#include <taglib/tag.h>
#include <taglib/tpropertymap.h>
#include <taglib/tstring.h>
#include <taglib/vorbisfile.h>

#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QMap>
#include <QPixmap>
#include <QString>
#include <QStringList>

#include <filesystem>
#include <string>
#include <vector>

namespace utils::audio
{
namespace fs = std::filesystem;

/// 路径转换: 在 Windows 平台上标准库方面使用 std::wstring
fs::path to_fs_path(const QString& s);

/// 路径转换: 在 Windows 平台上标准库方面使用 std::wstring
QString from_fs_path(const fs::path& p);

/// TagLib file path holder.
/// TagLib::FileName is a raw pointer (const char* or const wchar_t*),
/// so the string data must outlive all TagLib operations on the file.
/// This holder owns the converted string and implicitly converts to
/// TagLib::FileName for seamless use with TagLib constructors.
///
/// On Windows outside MSYS2, the C runtime's fopen() uses the system
/// ANSI code page (e.g. GBK), so UTF-8 paths fail.  TagLib supports
/// wide-char constructors — use them to bypass encoding issues entirely.
struct TagLibFileNameHolder
{
#ifdef Q_OS_WIN
    std::wstring storage;
#else
    std::string storage;
#endif

    explicit TagLibFileNameHolder(const QString& path)
    {
#ifdef Q_OS_WIN
        storage = path.toStdWString();
#else
        storage = path.toUtf8().toStdString();
#endif
    }

    operator TagLib::FileName() const
    {
#ifdef Q_OS_WIN
        return storage.c_str();
#else
        return storage.c_str();
#endif
    }
};

// property check
bool is_audio_file(const fs::path& path);
bool is_playlist(const fs::path& path);

/// 递归扫描路径中所有可用的文件, 默认忽略无法访问的路径
std::vector<fs::path> find_all(const QString& root_dir);
/// 解析形如 "disc/track" 结构的字符串, 并返回数字, 失败时返回 {0, 0}
std::pair<int, int> parse_disc_number(const std::string& str);
QPixmap find_cover_at_folder(const QString& audio_path);
QPixmap parse_cover_to_qpixmap(const QString& filepath);
bool taglib_writeback(const QString& filepath, const QMap<QString, QStringList>& tags);
QMap<QString, QStringList> parse_meta_to_map(const QString& filepath);
TrackMetaData parse_to_local_meta(const QString& filepath);
TrackMetaData format(TrackMetaData meta);

} // namespace utils::audio
