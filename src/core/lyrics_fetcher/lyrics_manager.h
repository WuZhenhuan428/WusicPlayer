#pragma once

#include <QString>
#include <QVector>

class QNetworkAccessManager;

namespace lyrics_fetcher
{

struct TrackMeta
{
    QString raw_title;
    QString raw_artist;
    QString raw_album;
    int duration_sec = 0;
};

struct LyricMeta
{
    QString title;
    QString artist;
    QString album;
    QString lyric_text;
    QString source;
};

class LyricsSink
{
public:
    LyricsSink()                                  = default;
    virtual ~LyricsSink()                         = default;
    LyricsSink(const LyricsSink&)                 = delete;
    LyricsSink& operator=(const LyricsSink&)      = delete;
    LyricsSink(LyricsSink&&) noexcept             = delete;
    LyricsSink& operator=(LyricsSink&&) noexcept  = delete;

    virtual LyricMeta create_lyric()              = 0;
    virtual void add_lyric(const LyricMeta& meta) = 0;
};

class LyricsManager
{
public:
    enum class Platform
    {
        Netease,
    };

    LyricsManager();

    QVector<LyricMeta> fetch(const TrackMeta& meta, QNetworkAccessManager* nam,
                             const QVector<Platform>& platforms = {}) const;

private:
    struct PlatformEntry
    {
        Platform platform;
        void (*fetch_fn)(const TrackMeta&, LyricsSink&, QNetworkAccessManager*);
    };
    QVector<PlatformEntry> entries_;
};

} // namespace lyrics_fetcher
