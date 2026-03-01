#pragma once

#include "core/types.h"

#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

#include <taglib/tag.h>
#include <taglib/id3v2tag.h>
#include <taglib/id3v2header.h>
#include <taglib/id3v2frame.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/fileref.h>
#include <taglib/tpropertymap.h>
#include <taglib/tstring.h>
#include <taglib/mpegfile.h>
#include <taglib/flacfile.h>
#include <taglib/oggflacfile.h>
#include <taglib/vorbisfile.h>
#include <taglib/opusfile.h>
#include <taglib/mp4file.h>
#include <taglib/asffile.h>

#include <QString>
#include <QPixmap>
#include <QFileInfo>
#include <QDir>
#include <QFileInfo>
#include <QStringList>
#include <QImageReader>

namespace fs = std::filesystem;

class AudioUtils {
public:
    // 扫描并返回所有音频文件和播放列表的路径
    static std::vector<fs::path> findAll(const std::string& rootDir) {
        std::vector<fs::path> results;
        
        auto scanDir = [](const fs::path& dir, std::vector<fs::path>& results) {
        try {
            for (const auto& entry : fs::recursive_directory_iterator(dir)) {
                if (entry.is_regular_file()) {
                    if (isAudioFile(entry.path()) || isPlaylist(entry.path())) {
                        results.push_back(entry.path());
                    }
                }
            }
        } catch (...) {
            // 忽略访问错误
        }
    };
        scanDir(fs::path(rootDir), results);
        return results;
    }
    
    // 判断是否是音频文件
    static bool isAudioFile(const fs::path& path) {
        if (!fs::is_regular_file(path)) return false;
        
        static const std::vector<std::string> audioExts = {
            ".mp3", ".wav", ".flac", ".aac", ".m4a",
            ".ogg", ".wma", ".opus", ".alac", ".aiff"
        };
        
        std::string ext = path.extension().string();
        for (char &c : ext) c = std::tolower(c);
        
        for (const auto& audioExt : audioExts) {
            if (ext == audioExt) return true;
        }
        
        return false;
    }
    
    // 判断是否是播放列表
    static bool isPlaylist(const fs::path& path) {
        if (!fs::is_regular_file(path)) return false;
        
        static const std::vector<std::string> playlistExts = {
            ".m3u", ".m3u8", ".pls", ".xspf"
        };
        
        std::string ext = path.extension().string();
        for (char &c : ext) c = std::tolower(c);
        
        for (const auto& playlistExt : playlistExts) {
            if (ext == playlistExt) return true;
        }
        
        return false;
    }

    static std::pair<int, int> parse_disc_number(const std::string& str) {
        if (str.empty()) {
            return {0, 0};
        }

        size_t slash = str.find("/");

        if (slash == std::string::npos) {
            try {
                return {std::stoi(str), 0};
            } catch (...) {
                return {0, 0};
            }
        }
        try {
            int num = std::stoi(str.substr(0, slash));
            int den = std::stoi(str.substr(slash+1));
            return {num, den};
        } catch (...) {
            return {0, 0};
        }
        return {0, 0};
    }

    static QPixmap find_cover_at_folder(const QString& audio_path) {
        QFileInfo audio_file(audio_path);
        if (!audio_file.exists()) {
            qDebug() << "[WARNING] audio file does not exist: " << audio_file;
            return QPixmap();
        }

        QDir audio_dir = audio_file.absoluteDir();
        if (!audio_dir.exists()) {
            qDebug() << "[WARNING] audio path does not exist: " << audio_dir;
            return QPixmap();
        }

        QVector<QString> support_formats;
        foreach (const QByteArray & format, QImageReader::supportedImageFormats()) {
            support_formats << "*." + QString(format).toLower();
        }

        QVector<QString> name_patterns = {
            "cover*", "folder*", "album*", "front*", "artwork*", "albumart*",
            "*cover*", "*folder*"   // <- files which contains keyword
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
            qDebug() << "[INFO] can not find any cover image.";
            return QPixmap();
        }

        std::sort(files.begin(), files.end(), [](const QString& a, const QString& b) {
            QString a_lower = a.toLower();
            QString b_lower = b.toLower();

            static const QVector<QString> priority_order = {
                "cover", "folder", "album", "front"
            };

            auto get_priority = [&](const QString& filename) -> int {
                for (int i = 0; i < priority_order.size(); ++i) {
                    if (filename.contains(priority_order.at(i))) {
                        return i;
                    }
                }
                return priority_order.size();
            };
            return get_priority(a_lower) < get_priority(b_lower);
        });

        QString cover_path = audio_dir.absoluteFilePath(files.first());
        qDebug() << "[INFO] Find default cover " << cover_path;
        QPixmap pix;
        pix.load(cover_path);
        return pix;
    }

    static QPixmap parse_cover_to_qpixmap(const std::string& filepath) {
        fs::path path(filepath);
        std::string ext = path.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });

        auto loadFromPicture = [](const TagLib::ByteVector& data) -> QPixmap {
            QPixmap pixmap;
            pixmap.loadFromData(reinterpret_cast<const uchar*>(data.data()), data.size());
            return pixmap;
        };

        auto pickId3v2Cover = [](TagLib::ID3v2::Tag* tag) -> QPixmap {
            if (!tag) {
                return QPixmap();
            }

            TagLib::ID3v2::FrameList frames = tag->frameList("APIC");
            if (frames.isEmpty()) {
                frames = tag->frameList("PIC");
            }
            if (frames.isEmpty()) {
                return QPixmap();
            }

            TagLib::ID3v2::AttachedPictureFrame* selected = nullptr;
            for (auto* frame : frames) {
                auto* picFrame = static_cast<TagLib::ID3v2::AttachedPictureFrame*>(frame);
                if (picFrame->type() == TagLib::ID3v2::AttachedPictureFrame::FrontCover) {
                    selected = picFrame;
                    break;
                }
            }
            if (!selected) {
                selected = static_cast<TagLib::ID3v2::AttachedPictureFrame*>(frames.front());
            }
            if (!selected) {
                return QPixmap();
            }

            QPixmap pixmap;
            pixmap.loadFromData(
                reinterpret_cast<const uchar*>(selected->picture().data()),
                selected->picture().size()
            );
            return pixmap;
        };

        QPixmap extracted_pixmap;

        if (ext == ".mp3") {
            TagLib::MPEG::File file(filepath.c_str());
            if (file.isValid() && file.hasID3v2Tag()) {
                extracted_pixmap =  pickId3v2Cover(file.ID3v2Tag());
            }
        } else if (ext == ".flac") {
            TagLib::FLAC::File file(filepath.c_str());
            const auto& pictures = file.pictureList();
            if (!pictures.isEmpty()) {
                extracted_pixmap =  loadFromPicture(pictures.front()->data());
            }
        } else if (ext == ".ogg" || ext == ".oga") {
            TagLib::Ogg::Vorbis::File file(filepath.c_str());
            if (file.isValid() && file.tag()) {
                const auto& pictures = file.tag()->pictureList();
                if (!pictures.isEmpty()) {
                    extracted_pixmap =  loadFromPicture(pictures.front()->data());
                }
            }
        } else if (ext == ".opus") {
            TagLib::Ogg::Opus::File file(filepath.c_str());
            if (file.isValid() && file.tag()) {
                const auto& pictures = file.tag()->pictureList();
                if (!pictures.isEmpty()) {
                    extracted_pixmap =  loadFromPicture(pictures.front()->data());
                }
            }
        } else if (ext == ".oga" || ext == ".ogx") {
            TagLib::Ogg::FLAC::File file(filepath.c_str());
            if (file.isValid() && file.tag()) {
                const auto& pictures = file.tag()->pictureList();
                if (!pictures.isEmpty()) {
                    extracted_pixmap =  loadFromPicture(pictures.front()->data());
                }
            }
        } else if (ext == ".m4a" || ext == ".mp4" || ext == ".aac") {
            TagLib::MP4::File file(filepath.c_str());
            if (file.isValid() && file.tag()) {
                TagLib::MP4::ItemMap items = file.tag()->itemMap();
                if (items.contains("covr")) {
                    TagLib::MP4::CoverArtList covers = items["covr"].toCoverArtList();
                    if (!covers.isEmpty()) {
                        extracted_pixmap =  loadFromPicture(covers.front().data());
                    }
                }
            }
        } else if (ext == ".wma") {
            TagLib::ASF::File file(filepath.c_str());
            if (file.isValid() && file.tag()) {
                const auto& attrs = file.tag()->attributeListMap()["WM/Picture"];
                if (!attrs.isEmpty()) {
                    TagLib::ASF::Picture pic = attrs.front().toPicture();
                    extracted_pixmap =  loadFromPicture(pic.picture());
                }
            }
        }

        if (!extracted_pixmap.isNull()) {
            return extracted_pixmap;
        }

        return AudioUtils::find_cover_at_folder(QString::fromStdString(filepath));
    }

    static TrackMetaData parse(const std::string& filepath) {
        TrackMetaData meta;

        TagLib::FileRef f(filepath.c_str());
        if (f.isNull() || !f.tag() || !f.audioProperties()) {
            meta.isValid = false;
            return meta;
        }

        QFileInfo ff(QString::fromStdString(filepath));
        meta.filepath = QString::fromStdString(filepath);
        meta.filename = ff.fileName();

        TagLib::Tag* tag = f.tag();
        // basic properties
        meta.album = QString::fromStdString(tag->album().to8Bit(true));
        meta.title = QString::fromStdString(tag->title().to8Bit(true));
        meta.artist = QString::fromStdString(tag->artist().to8Bit(true));
        meta.comment = QString::fromStdString(tag->comment().to8Bit(true));
        meta.genre = QString::fromStdString(tag->genre().to8Bit(true));
        meta.track_number = tag->track();
        meta.year = tag->year();

        meta.duration_s = f.audioProperties()->lengthInSeconds();

        // @todo: file type parse
        
        TagLib::PropertyMap props = f.file()->properties();

        // get extend fileds
        auto getString = [&](const char* key) -> std::string {
            if (props.contains(key) && !props[key].isEmpty()) {
                return props[key].front().to8Bit(true);
            }
            return "";
        };

        meta.album_artist = QString::fromStdString(getString("ALBUMARTIST"));
        if (meta.album_artist.isEmpty()) {
            meta.album_artist = QString::fromStdString(getString("ALBUM ARTIST"));
        }
        if (meta.album_artist.isEmpty()) {
            // qDebug() << "[INFO] AudioUtils::parse album_artist is empty!";
        }

        meta.lyrics = QString::fromStdString(getString("LYRICS"));

        std::string disc_str = getString("DISCNUMBER");
        auto [num, total] = parse_disc_number(disc_str);
        meta.disc_number = num;
        meta.disc_total = total;

        meta.isValid = true;
        return meta;
    }

    static TrackMetaData format(TrackMetaData meta) {
        TrackMetaData temp = meta;
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
};

// // 使用例
// int main() {
//     std::string dir = "/mnt/win_d/MUSIC/MUSIC/MintJam/ONE";
//     auto files = AudioUtils::findAll(dir);
    
//     std::cout << "找到 " << files.size() << " 个音频相关文件:\n" << std::endl;
    
//     for (const auto& file : files) {
//         if (AudioUtils::isAudioFile(file)) {
//             std::cout << "[AUDIO] ";
//         } else if (AudioUtils::isPlaylist(file)) {
//             std::cout << "[PLAYLIST] ";
//         }
//         std::cout << file.filename().string() 
//                   << " (" << fs::file_size(file) << " bytes)" << std::endl;
//     }
    
//     return 0;
// }
