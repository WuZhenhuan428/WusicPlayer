/*
- Ported from https://github.com/ESLyric/scripts/blob/main/searcher/netease.js to Qt6 C++.
- There is no license in the source repository.
- If there are any issues about license, please contact me via email: wuzhenhuan545@gmail.com
*/

#pragma once

/*
使用方法（简版）
1) 使用 lyrics_fetcher::LyricsManager 管理多平台歌词请求。
2) 创建并复用 QNetworkAccessManager 实例。
3) 调用 getLyrics(meta, sink, nam) 拉取并回调歌词。

示例：
*/

#include "core/lyrics_fetcher/lyrics_manager.h"

#include <QString>

class QNetworkAccessManager;

namespace netease_qt6
{

struct Config
{
    QString name;
    QString version;
    QString author;
};

// 返回搜索器基础信息（名称、版本、作者）。
Config get_config();

// 根据歌曲元数据请求网易云歌词，并通过 manager 回调输出候选结果。
void get_lyrics(const lyrics_fetcher::TrackMeta& meta, lyrics_fetcher::LyricsSink& sink,
                QNetworkAccessManager* nam);

} // namespace netease_qt6
