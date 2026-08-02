#include "core/utils/audio.hpp"

#include <QString>
#include <print>
#include <string>

int main()
{
    std::string dir = "/mnt/win_c/MUSIC/MintJam/ONE";
    auto files      = utils::audio::find_all(QString::fromStdString(dir));

    // 1. find audio files
    std::println("There is {} files under {}:", files.size(), dir);
    for (const auto& file : files) {
        std::println("\t{}", file.string());
    }
}
