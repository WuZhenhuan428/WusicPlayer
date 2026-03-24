#pragma once

#include "types.h"
#include <QVector>

class QString;

struct SearchHint
{
    trackId track_id;
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
    playlistId pid;
    bool case_sensitive = false;
};