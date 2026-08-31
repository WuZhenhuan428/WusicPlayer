#pragma once

#include <QString>
#include <QVector>

class QNetworkAccessManager;

namespace lyrics_fetcher
{

struct TrackMeta
{
    QString rawTitle;
    QString rawArtist;
    QString rawAlbum;
    int durationSec = 0;
};

struct LyricMeta
{
    QString title;
    QString artist;
    QString album;
    QString lyricText;
    QString source;
};

class LyricsSink
{
public:
    virtual ~LyricsSink()                         = default;
    virtual LyricMeta create_lyric()              = 0;
    virtual void add_lyric(const LyricMeta& meta) = 0;
};

class LyricsManager
{
public:
    enum class Platform
    {
        Netease
    };

    LyricsManager();

    QVector<LyricMeta> fetch(const TrackMeta& meta, QNetworkAccessManager* nam,
                             const QVector<Platform>& platforms = {}) const;

private:
    struct PlatformEntry
    {
        Platform platform;
        void (*fetchFn)(const TrackMeta&, LyricsSink&, QNetworkAccessManager*);
    };
    QVector<PlatformEntry> m_entries;
};

} // namespace lyrics_fetcher
