#include "core/lyrics_fetcher/lyrics_manager.h"

#include "netease_qt6.h"

namespace lyrics_fetcher
{

namespace
{
class CollectSink final : public LyricsSink
{
public:
    LyricMeta create_lyric() override
    {
        return LyricMeta{};
    }

    void add_lyric(const LyricMeta& meta) override
    {
        items_.push_back(meta);
    }

    QVector<LyricMeta> take()
    {
        return std::move(items_);
    }

private:
    QVector<LyricMeta> items_;
};
} // namespace

LyricsManager::LyricsManager()
{
    entries_.push_back({Platform::Netease, &netease_qt6::get_lyrics});
}

QVector<LyricMeta> LyricsManager::fetch(const TrackMeta& meta, QNetworkAccessManager* nam,
                                        const QVector<Platform>& platforms) const
{
    QVector<LyricMeta> all;
    if (!nam) {
        return all;
    }

    CollectSink sink;
    for (const auto& entry : entries_) {
        if (!platforms.isEmpty() && !platforms.contains(entry.platform)) {
            continue;
        }
        entry.fetch_fn(meta, sink, nam);
    }

    all = sink.take();
    return all;
}

} // namespace lyrics_fetcher
