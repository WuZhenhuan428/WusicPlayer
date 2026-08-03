#pragma once

#include "core/lyrics_fetcher/lyrics_manager.h"

class QNetworkAccessManager;

namespace qqmusic_qt6 {

struct Config {
    QString name;
    QString version;
    QString author;
};

Config getConfig();
void getLyrics(const lyrics_fetcher::TrackMeta& meta,
               lyrics_fetcher::LyricsSink& sink,
               QNetworkAccessManager* nam);

} // namespace qqmusic_qt6
