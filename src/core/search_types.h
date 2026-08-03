#pragma once

#include "core/types.h"
#include <QVector>

class QString;

struct SearchHint
{
    TrackId track_id; // 库级曲目身份(库搜索/库引用条目);外部条目为空
    QString filepath; // 播放依据(播放列表条目/外部条目必有;库条目可一并携带)
    QString title;
    QString artist;
    QString album_artist;
    QString album;
    int duration_s;
    unsigned char score;
};

enum class SearchQueryMode
{
    Plain,
    Fuzzy,
    Prefix
};

struct SearchQuery
{
    QString keyword;
    SearchQueryMode mode = SearchQueryMode::Plain;
    PlaylistId pid;
    bool case_sensitive = false;
};
