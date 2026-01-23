#include <iostream>
#include <filesystem>
#include <vector>
#include <string>

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

// 使用例
int main() {
    std::string dir = "/mnt/win_d/MUSIC/MUSIC/MintJam/ONE";
    auto files = Audio::findAll(dir);
    
    std::cout << "找到 " << files.size() << " 个音频相关文件:\n" << std::endl;
    
    for (const auto& file : files) {
        if (Audio::isAudioFile(file)) {
            std::cout << "[AUDIO] ";
        } else if (Audio::isPlaylist(file)) {
            std::cout << "[PLAYLIST] ";
        }
        std::cout << file.string() 
                  << " (" << fs::file_size(file) << " bytes)" << std::endl;
    }
    
    return 0;
}