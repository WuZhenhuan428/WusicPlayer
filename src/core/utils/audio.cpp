#include "audio.hpp"

#include "core/utils/string.hpp"
#include <algorithm>
#include <ranges>

#include "core/logger/logger_manager.h"
namespace
{
// NOLINTNEXTLINE
Logger* audio_utils_logger = LoggerManager::file_logger("audio_utils", {"console", "gui"});
} // namespace

namespace utils::audio
{
namespace fs = std::filesystem;

// --- helpers: fs::path <-> QString (encoding-safe across platforms) ---
// On Windows with MSVC, std::filesystem uses wide chars internally;
// with MinGW it uses UTF-8 narrow chars. These helpers unify the two.
fs::path to_fs_path(const QString& s)
{
#ifdef Q_OS_WIN
    return {s.toStdWString()};
#else
    return {s.toStdString()};
#endif
}

QString from_fs_path(const fs::path& p)
{
#ifdef Q_OS_WIN
    return QString::fromStdWString(p.wstring());
#else
    return QString::fromStdString(p.string());
#endif
}

bool is_audio_file(const fs::path& path)
{
    if (!fs::is_regular_file(path)) {
        return false;
    }

    static const std::vector<std::string> audio_exts = {
        ".mp3", ".wav", ".flac", ".aac", ".m4a", ".ogg", ".wma", ".opus", ".alac", ".aiff",
    };

    std::string ext = path.extension().string();
    for (char& c : ext) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    return std::ranges::any_of(audio_exts,
                               [&ext](const auto& audio_ext) { return ext == audio_ext; });
}

bool is_playlist(const fs::path& path)
{
    if (!fs::is_regular_file(path)) {
        return false;
    }

    static const std::vector<std::string> playlist_exts = {".m3u", ".m3u8", ".pls", ".xspf"};

    std::string ext                                     = path.extension().string();
    for (char& c : ext) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    return std::ranges::any_of(playlist_exts, [&ext](const auto& pl_ext) { return ext == pl_ext; });
}

std::vector<fs::path> find_all(const QString& root_dir)
{
    std::vector<fs::path> results;
    const fs::path root_path = to_fs_path(root_dir);

    auto scan_dir            = [](const fs::path& dir, std::vector<fs::path>& results) {
        std::error_code ec;
        for (const auto& entry : fs::recursive_directory_iterator(dir, ec)) {
            if (ec) {
                audio_utils_logger->warn("find_all: failed to visit: {}, error_code message: {}",
                                         entry.path().string(), ec.message());
                continue; // 忽略访问错误
            }
            if (entry.is_regular_file()) {
                if (is_audio_file(entry.path()) || is_playlist(entry.path())) {
                    results.push_back(entry.path());
                }
            }
        }
    };
    scan_dir(root_path, results);
    return results;
}

std::pair<int, int> parse_disc_number(const std::string& str)
{
    if (str.empty()) {
        return {0, 0};
    }

    size_t slash = str.find('/');

    if (slash == std::string::npos) {
        auto value = utils::string::string_to_int(str);
        if (value.has_value()) {
            return {*value, 0};
        }
        return {0, 0};
    }

    auto num = utils::string::string_to_int(str.substr(0, slash));
    auto den = utils::string::string_to_int(str.substr(slash + 1));
    if (!num.has_value() || !den.has_value()) {
        return {0, 0};
    }
    return {*num, *den};
}

QPixmap find_cover_at_folder(const QString& audio_path)
{
    QFileInfo audio_file(audio_path);
    if (!audio_file.exists()) {
        audio_utils_logger->warn("audio file does not exist: {}", audio_file.fileName());
        return {};
    }

    QDir audio_dir = audio_file.absoluteDir();
    if (!audio_dir.exists()) {
        audio_utils_logger->warn("audio path does not exist: {}", audio_dir.dirName());
        return {};
    }

    QVector<QString> support_formats;
    foreach (const QByteArray& format, QImageReader::supportedImageFormats()) {
        support_formats << "*." + QString(format).toLower();
    }

    QVector<QString> name_patterns = {
        "cover*",   "folder*",   "album*",  "front*",
        "artwork*", "albumart*", "*cover*", "*folder*", // <- files which contains keyword
    };

    // build search pattern
    QVector<QString> search_patterns;
    for (const QString& pattern : name_patterns) {
        for (const QString& format : support_formats) {
            search_patterns << (pattern + format);
        }
    }

    // set name filter (ignore case)
    audio_dir.setNameFilters(search_patterns);
    audio_dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);

    QVector<QString> files = audio_dir.entryList();
    if (files.isEmpty()) {
        audio_utils_logger->info("can not find any cover image");
        return {};
    }

    std::ranges::sort(files, [](const QString& a, const QString& b) {
        QString a_lower                              = a.toLower();
        QString b_lower                              = b.toLower();

        static const QVector<QString> priority_order = {"cover", "folder", "album", "front"};

        auto get_priority                            = [&](const QString& filename) -> int {
            for (int i = 0; i < priority_order.size(); ++i) {
                if (filename.contains(priority_order.at(i))) {
                    return i;
                }
            }
            return static_cast<int>(priority_order.size());
        };
        return get_priority(a_lower) < get_priority(b_lower);
    });

    QString cover_path = audio_dir.absoluteFilePath(files.first());
    audio_utils_logger->info("Find default cover");
    QPixmap pix;
    pix.load(cover_path);
    return pix;
}

QPixmap parse_cover_to_qpixmap(const QString& filepath)
{
    TagLibFileNameHolder fname(filepath);
    fs::path path   = to_fs_path(filepath);
    std::string ext = path.extension().string();
    std::ranges::transform(ext, ext.begin(), [](unsigned char c) { return std::tolower(c); });

    auto load_from_picture = [](const TagLib::ByteVector& data) -> QPixmap {
        QPixmap pixmap;
        pixmap.loadFromData(reinterpret_cast<const uchar*>(data.data()), data.size());
        return pixmap;
    };

    auto pick_id3v2_cover = [](TagLib::ID3v2::Tag* tag) -> QPixmap {
        if (!tag) {
            return {};
        }

        TagLib::ID3v2::FrameList frames = tag->frameList("APIC");
        if (frames.isEmpty()) {
            frames = tag->frameList("PIC");
        }
        if (frames.isEmpty()) {
            return {};
        }

        TagLib::ID3v2::AttachedPictureFrame* selected = nullptr;
        for (auto* frame : frames) {
            auto* pic_frame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(frame);
            if (pic_frame->type() == TagLib::ID3v2::AttachedPictureFrame::FrontCover) {
                selected = pic_frame;
                break;
            }
        }
        if (!selected) {
            selected = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(frames.front());
        }
        if (!selected) {
            return {};
        }

        QPixmap pixmap;
        pixmap.loadFromData(reinterpret_cast<const uchar*>(selected->picture().data()),
                            selected->picture().size());
        return pixmap;
    };

    QPixmap extracted_pixmap;

    if (ext == ".mp3") {
        TagLib::MPEG::File file(fname);
        if (file.isValid() && file.hasID3v2Tag()) {
            extracted_pixmap = pick_id3v2_cover(file.ID3v2Tag());
        }
    } else if (ext == ".flac") {
        TagLib::FLAC::File file(fname);
        const auto& pictures = file.pictureList();
        if (!pictures.isEmpty()) {
            extracted_pixmap = load_from_picture(pictures.front()->data());
        }
    } else if (ext == ".ogg" || ext == ".oga") {
        TagLib::Ogg::Vorbis::File file(fname);
        if (file.isValid() && file.tag()) {
            const auto& pictures = file.tag()->pictureList();
            if (!pictures.isEmpty()) {
                extracted_pixmap = load_from_picture(pictures.front()->data());
            }
        }
    } else if (ext == ".opus") {
        TagLib::Ogg::Opus::File file(fname);
        if (file.isValid() && file.tag()) {
            const auto& pictures = file.tag()->pictureList();
            if (!pictures.isEmpty()) {
                extracted_pixmap = load_from_picture(pictures.front()->data());
            }
        }
    } else if (ext == ".oga" || ext == ".ogx") {
        TagLib::Ogg::FLAC::File file(fname);
        if (file.isValid() && file.tag()) {
            const auto& pictures = file.tag()->pictureList();
            if (!pictures.isEmpty()) {
                extracted_pixmap = load_from_picture(pictures.front()->data());
            }
        }
    } else if (ext == ".m4a" || ext == ".mp4" || ext == ".aac") {
        TagLib::MP4::File file(fname);
        if (file.isValid() && file.tag()) {
            TagLib::MP4::ItemMap items = file.tag()->itemMap();
            if (items.contains("covr")) {
                TagLib::MP4::CoverArtList covers = items["covr"].toCoverArtList();
                if (!covers.isEmpty()) {
                    extracted_pixmap = load_from_picture(covers.front().data());
                }
            }
        }
    } else if (ext == ".wma") {
        TagLib::ASF::File file(fname);
        if (file.isValid() && file.tag()) {
            const auto& attrs = file.tag()->attributeListMap()["WM/Picture"];
            if (!attrs.isEmpty()) {
                TagLib::ASF::Picture pic = attrs.front().toPicture();
                extracted_pixmap         = load_from_picture(pic.picture());
            }
        }
    }

    if (!extracted_pixmap.isNull()) {
        return extracted_pixmap;
    }

    return audio::find_cover_at_folder(filepath);
}

bool taglib_writeback(const QString& filepath, const QMap<QString, QStringList>& tags)
{
    TagLibFileNameHolder fname(filepath);
    TagLib::FileRef f(fname);
    if (f.isNull() || !f.file()) {
        return false;
    }

    if (tags.isEmpty()) {
        return false;
    }

    auto normalize_key = [](QString key) -> QString {
        key = key.trimmed();
        if (key.startsWith('<') && key.endsWith('>') && key.size() > 2) {
            key = key.mid(1, key.size() - 2);
        }

        QString normalized;
        normalized.reserve(key.size());
        for (const QChar& ch : key) {
            if (ch == '_' || ch == '-' || ch == ' ') {
                continue;
            }
            normalized.append(ch.toUpper());
        }
        return normalized;
    };

    TagLib::PropertyMap props = f.file()->properties();
    for (auto it = tags.constBegin(); it != tags.constEnd(); ++it) {
        const QString normalized_key = normalize_key(it.key());
        if (normalized_key.isEmpty()) {
            continue;
        }

        const TagLib::String tag_key(normalized_key.toUtf8().constData(), TagLib::String::Latin1);
        const auto& values = it.value();
        TagLib::StringList list;

        for (const QString& value : values) {
            const QString cleaned = value.trimmed();
            if (!cleaned.isEmpty()) {
                list.append(TagLib::String(cleaned.toUtf8().constData(), TagLib::String::UTF8));
            }
        }

        props.erase(tag_key);
        if (!list.isEmpty()) {
            props.insert(tag_key, list);
        }
    }

    f.setProperties(props);
    return f.file()->save();
}

QMap<QString, QStringList> parse_meta_to_map(const QString& filepath)
{
    QMap<QString, QStringList> result;

    auto normalize_key = [](const QString& key) -> QString {
        QString normalized;
        normalized.reserve(key.size());

        for (const QChar& ch : key) {
            if (ch == '_' || ch == '-' || ch == ' ') {
                continue;
            }
            normalized.append(ch.toUpper());
        }
        return normalized;
    };

    TagLibFileNameHolder fname(filepath);
    TagLib::FileRef f(fname);
    if (f.isNull() || !f.tag()) {
        return {};
    }

    TagLib::PropertyMap props = f.file()->properties();

    for (const auto& [raw, attrs] : props) {
        // toCString(true) returns a null-terminated UTF-8 string.
        // QString::fromUtf8() decodes correctly on all platforms
        // (unlike fromStdString, which uses the local ANSI code page on Windows).
        QString key = QString::fromUtf8(raw.toCString(true));
        QStringList values;

        for (const auto& a : attrs) {
            values << QString::fromUtf8(a.toCString(true));
        }

        // Keep both original key and normalized key for compatibility.
        result.insert(key, values);
        result.insert(normalize_key(key), values);
    }
    return result;
}

TrackMetaData parse_to_local_meta(const QString& filepath)
{
    TrackMetaData meta;
    TagLibFileNameHolder fname(filepath);

    const auto map = parse_meta_to_map(filepath);
    QFileInfo ff(filepath);
    meta.filepath              = filepath;
    meta.filename              = ff.fileName();

    auto normalize_multi_props = [&](const QString& key) -> QString {
        const QStringList values = map.value(key);
        if (!values.isEmpty()) {
            QString temp;
            for (int i = 0; i < values.size(); ++i) {
                temp += values.at(i);
                if (i < (values.size() - 1)) {
                    temp += u",";
                }
            }
            return temp;
        }
        return QString{};
    };

    auto parse_leading_int = [](QString value) -> int {
        value = value.trimmed();
        if (value.isEmpty()) {
            return 0;
        }

        // Handle values like "3/12".
        const ssize_t slash = static_cast<ssize_t>(value.indexOf('/'));
        if (slash > 0) {
            value = value.left(slash).trimmed();
        }

        QString digits;
        for (const QChar& ch : value) {
            if (ch.isDigit()) {
                digits.append(ch);
            } else {
                break;
            }
        }

        bool ok    = false;
        int number = digits.toInt(&ok);
        return ok ? number : 0;
    };

    // basic properties
    meta.album        = normalize_multi_props("ALBUM");
    meta.title        = normalize_multi_props("TITLE");
    meta.artist       = normalize_multi_props("ARTIST");
    meta.comment      = normalize_multi_props("COMMENT");
    meta.genre        = normalize_multi_props("GENRE");
    meta.track_number = parse_leading_int(normalize_multi_props("TRACKNUMBER"));
    meta.year         = parse_leading_int(normalize_multi_props("DATE"));
    meta.duration_s = static_cast<int>(parse_leading_int(normalize_multi_props("LENGTH")) / 1000.0);
    meta.album_artist    = normalize_multi_props("ALBUMARTIST");
    meta.lyrics          = normalize_multi_props("LYRICS");

    std::string disc_str = normalize_multi_props("DISCNUMBER").toStdString();
    auto [num, total]    = parse_disc_number(disc_str);
    meta.disc_number     = num;
    meta.disc_total      = total;

    // Fallback duration from audio properties when metadata map has no length.
    if (meta.duration_s <= 0) {
        TagLib::FileRef f(fname);
        if (!f.isNull() && f.audioProperties()) {
            meta.duration_s = f.audioProperties()->lengthInSeconds();
        }
    }

    meta.is_valid = ff.exists();
    return meta;
}

TrackMetaData format(TrackMetaData meta)
{
    TrackMetaData temp = std::move(meta);
    if (temp.album.isEmpty()) {
        temp.album = "Unknown Album";
    }
    if (temp.title.isEmpty()) {
        temp.title = temp.filename;
    }
    if (temp.artist.isEmpty()) {
        temp.artist = "Unknown Artist";
    }
    if (temp.genre.isEmpty()) {
        temp.genre = "Unknown Genre";
    }
    return temp;
}

} // namespace utils::audio
