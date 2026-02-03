#pragma once

#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <sstream>

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

#include "../../src/playlist/playlist_definitions.h"

#include <QString>
#include <QPixmap>
#include <QFileInfo>

namespace fs = std::filesystem;

class Audio {
public:
    // 扫描并返回所有音频文件和播放列表的路径
    static std::vector<fs::path> findAll(const std::string& rootDir) {
        std::vector<fs::path> results;
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

    static QPixmap parse_cover_to_qpixmap(const std::string& filepath) {
        fs::path path(filepath);
        std::string ext = path.extension().string();
        // to lower
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });

        if (ext == ".mp3") {
            TagLib::MPEG::File file(filepath.c_str());
            if (file.isValid() && file.hasID3v2Tag()) {
                TagLib::ID3v2::Tag *tag = file.ID3v2Tag();
                // 尝试获取 APIC (ID3v2.3/2.4) 或 PIC (ID3v2.2) 帧
                TagLib::ID3v2::FrameList l = tag->frameList("APIC");
                if (l.isEmpty()) {
                    l = tag->frameList("PIC");
                }

                if(!l.isEmpty()) {
                    TagLib::ID3v2::AttachedPictureFrame *selectedFrame = nullptr;

                    // 1. 优先寻找类型为 FrontCover (0x03) 的图片
                    for(auto* frame : l) {
                        auto* picFrame = static_cast<TagLib::ID3v2::AttachedPictureFrame*>(frame);
                        if (picFrame->type() == TagLib::ID3v2::AttachedPictureFrame::FrontCover) {
                            selectedFrame = picFrame;
                            break;
                        }
                    }

                    // 2. 如果没找到 FrontCover，则默认使用第一张图片
                    if (!selectedFrame) {
                        selectedFrame = static_cast<TagLib::ID3v2::AttachedPictureFrame*>(l.front());
                    }

                    if (selectedFrame) {
                        QPixmap pixmap;
                        // 注意：这里需要确保数据有效性，loadFromData会自动探测格式
                        pixmap.loadFromData(
                            reinterpret_cast<const uchar*>(selectedFrame->picture().data()),
                            selectedFrame->picture().size()
                        );
                        return pixmap;
                    }
                }
            }
        } 
        else if (ext == ".flac") {
             TagLib::FLAC::File file(filepath.c_str());
             const TagLib::List<TagLib::FLAC::Picture*>& pictures = file.pictureList();
             if (!pictures.isEmpty()) {
                 TagLib::FLAC::Picture* pic = pictures.front();
                 QPixmap pixmap;
                 pixmap.loadFromData(
                     reinterpret_cast<const uchar*>(pic->data().data()),
                     pic->data().size()
                 );
                 return pixmap;
             }
        }

        return QPixmap();
    }

    static TrackMetaData parse(const std::string& filepath) {
        TrackMetaData meta;

        TagLib::FileRef f(filepath.c_str());
        if (f.isNull() || !f.tag() || !f.audioProperties()) {
            meta.isValid = false;
            return meta;
        }

        QFileInfo ff(QString::fromStdString(filepath));
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
            // qDebug() << "[INFO] Audio::parse album_artist is empty!";
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

private:
    static void scanDir(const fs::path& dir, std::vector<fs::path>& results) {
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
    }
};

// // 使用例
// int main() {
//     std::string dir = "/mnt/win_d/MUSIC/MUSIC/MintJam/ONE";
//     auto files = Audio::findAll(dir);
    
//     std::cout << "找到 " << files.size() << " 个音频相关文件:\n" << std::endl;
    
//     for (const auto& file : files) {
//         if (Audio::isAudioFile(file)) {
//             std::cout << "[AUDIO] ";
//         } else if (Audio::isPlaylist(file)) {
//             std::cout << "[PLAYLIST] ";
//         }
//         std::cout << file.filename().string() 
//                   << " (" << fs::file_size(file) << " bytes)" << std::endl;
//     }
    
//     return 0;
// }
