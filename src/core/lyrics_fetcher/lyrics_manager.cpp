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
        m_items.push_back(meta);
    }

    QVector<LyricMeta> take()
    {
        return std::move(m_items);
    }

private:
    QVector<LyricMeta> m_items;
};
} // namespace

LyricsManager::LyricsManager()
{
    m_entries.push_back({Platform::Netease, &netease_qt6::getLyrics});
}

QVector<LyricMeta> LyricsManager::fetch(const TrackMeta& meta, QNetworkAccessManager* nam,
                                        const QVector<Platform>& platforms) const
{
    QVector<LyricMeta> all;
    if (!nam) {
        return all;
    }

    CollectSink sink;
    for (const auto& entry : m_entries) {
        if (!platforms.isEmpty() && !platforms.contains(entry.platform)) {
            continue;
        }
        entry.fetchFn(meta, sink, nam);
    }

    all = sink.take();
    return all;
}

} // namespace lyrics_fetcher
