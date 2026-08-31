#include "qqmusic_qt6.h"

#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>

namespace qqmusic_qt6
{

namespace
{
struct SongEntry
{
    QString songmid;
    QString title;
    QString artist;
    QString album;
};

QByteArray doGet(const QUrl& url, QNetworkAccessManager* nam)
{
    if (!nam) {
        return {};
    }

    QNetworkRequest req(url);
    req.setRawHeader("Referer", "https://y.qq.com");
    req.setRawHeader("User-Agent", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, "
                                   "like Gecko) Chrome/120 Safari/537.36");

    QNetworkReply* reply = nam->get(req);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const bool ok         = reply->error() == QNetworkReply::NoError;
    const int status      = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QString errText = reply->errorString();
    const QByteArray body = reply->readAll();
    reply->deleteLater();
    if (!ok || status != 200) {
        qWarning() << "[QQMusic] GET failed" << url << "status=" << status << "error=" << errText;
        return {};
    }
    qDebug() << "[QQMusic] GET ok" << url << "bytes=" << body.size();
    return ok ? body : QByteArray{};
}

QByteArray doPostJson(const QUrl& url, const QJsonObject& payload, QNetworkAccessManager* nam)
{
    if (!nam) {
        return {};
    }

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Origin", "https://y.qq.com");
    req.setRawHeader("Referer", "https://y.qq.com");
    req.setRawHeader("User-Agent", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, "
                                   "like Gecko) Chrome/120 Safari/537.36");

    const QByteArray bodyData = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    QNetworkReply* reply      = nam->post(req, bodyData);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const bool ok         = reply->error() == QNetworkReply::NoError;
    const int status      = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QString errText = reply->errorString();
    const QByteArray body = reply->readAll();
    reply->deleteLater();
    if (!ok || status != 200) {
        qWarning() << "[QQMusic] POST failed" << url << "status=" << status << "error=" << errText;
        return {};
    }
    qDebug() << "[QQMusic] POST ok" << url << "bytes=" << body.size();
    return body;
}

QVector<SongEntry> parseSearchList(const QJsonObject& root)
{
    QVector<SongEntry> out;

    QJsonArray list =
        root.value("data").toObject().value("song").toObject().value("list").toArray();
    if (list.isEmpty()) {
        list = root.value("req_0")
                   .toObject()
                   .value("data")
                   .toObject()
                   .value("body")
                   .toObject()
                   .value("song")
                   .toObject()
                   .value("list")
                   .toArray();
    }

    for (const auto& item : list) {
        const QJsonObject song = item.toObject();
        SongEntry e;
        e.songmid = song.value("songmid").toString();
        if (e.songmid.isEmpty()) {
            e.songmid = song.value("mid").toString();
        }
        e.title = song.value("songname").toString();
        if (e.title.isEmpty()) {
            e.title = song.value("name").toString();
        }
        e.album = song.value("albumname").toString();
        if (e.album.isEmpty()) {
            e.album = song.value("album").toObject().value("name").toString();
        }

        const QJsonArray singers = song.value("singer").toArray();
        if (!singers.isEmpty()) {
            e.artist = singers.at(0).toObject().value("name").toString();
        }

        if (!e.songmid.isEmpty()) {
            out.push_back(e);
        }
    }

    return out;
}

QVector<SongEntry> searchSongsViaMusicu(const QString& keyword, QNetworkAccessManager* nam)
{
    QJsonObject payload;
    payload.insert("comm", QJsonObject{{"ct", 24},
                                       {"cv", 0},
                                       {"uin", "0"},
                                       {"format", "json"},
                                       {"inCharset", "utf-8"},
                                       {"outCharset", "utf-8"}});
    payload.insert("req_0", QJsonObject{{"module", "music.search.SearchCgiService"},
                                        {"method", "DoSearchForQQMusicDesktop"},
                                        {"param", QJsonObject{{"query", keyword},
                                                              {"num_per_page", 10},
                                                              {"page_num", 1},
                                                              {"search_type", 0}}}});

    const QByteArray body = doPostJson(QUrl("https://u.y.qq.com/cgi-bin/musicu.fcg"), payload, nam);
    if (body.isEmpty()) {
        return {};
    }

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[QQMusic] musicu parse failed" << err.errorString()
                   << "body=" << QString::fromUtf8(body.left(180));
        return {};
    }

    return parseSearchList(doc.object());
}

QVector<SongEntry> searchSongs(const lyrics_fetcher::TrackMeta& meta, QNetworkAccessManager* nam)
{
    const QString title   = meta.rawTitle.trimmed();
    const QString artist  = meta.rawArtist.trimmed();
    const QString keyword = artist.isEmpty() ? title : (title + "+" + artist);
    if (keyword.isEmpty()) {
        qWarning() << "[QQMusic] empty keyword";
        return {};
    }

    qDebug() << "[QQMusic] keyword=" << keyword;
    QVector<SongEntry> out = searchSongsViaMusicu(keyword, nam);
    if (!out.isEmpty()) {
        qDebug() << "[QQMusic] search candidates=" << out.size() << "via musicu";
        return out;
    }

    auto buildLegacySearchUrl = [&](const QString& scheme, const QString& queryWord) {
        QUrlQuery q;
        q.addQueryItem("format", "json");
        q.addQueryItem("n", "10");
        q.addQueryItem("p", "0");
        q.addQueryItem("w", queryWord);
        q.addQueryItem("cr", "1");
        q.addQueryItem("g_tk", "5381");

        QUrl url(QString("%1://c.y.qq.com/soso/fcgi-bin/client_search_cp").arg(scheme));
        url.setQuery(q);
        return url;
    };

    QByteArray body = doGet(buildLegacySearchUrl("https", keyword), nam);
    if (body.isEmpty()) {
        qDebug() << "[QQMusic] retry legacy search via http";
        body = doGet(buildLegacySearchUrl("http", keyword), nam);
    }

    if (body.isEmpty() && !artist.isEmpty()) {
        qDebug() << "[QQMusic] retry legacy search with title only";
        body = doGet(buildLegacySearchUrl("https", title), nam);
        if (body.isEmpty()) {
            body = doGet(buildLegacySearchUrl("http", title), nam);
        }
    }

    if (body.isEmpty()) {
        qWarning() << "[QQMusic] search body empty for query" << meta.rawTitle << meta.rawArtist;
        return out;
    }

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[QQMusic] search parse failed" << err.errorString()
                   << "body=" << QString::fromUtf8(body.left(180));
        return out;
    }

    out = parseSearchList(doc.object());

    qDebug() << "[QQMusic] search candidates=" << out.size();

    return out;
}

QString queryLyricBySongmid(const QString& songmid, QNetworkAccessManager* nam)
{
    QUrlQuery q;
    q.addQueryItem("songmid", songmid);
    q.addQueryItem("pcachetime", QString::number(QDateTime::currentMSecsSinceEpoch()));
    q.addQueryItem("g_tk", "5381");
    q.addQueryItem("loginUin", "0");
    q.addQueryItem("hostUin", "0");
    q.addQueryItem("inCharset", "utf8");
    q.addQueryItem("outCharset", "utf-8");
    q.addQueryItem("notice", "0");
    q.addQueryItem("platform", "yqq");
    q.addQueryItem("needNewCode", "1");
    q.addQueryItem("format", "json");

    QUrl url("https://c.y.qq.com/lyric/fcgi-bin/fcg_query_lyric_new.fcg");
    url.setQuery(q);

    const QByteArray body = doGet(url, nam);
    if (body.isEmpty()) {
        qWarning() << "[QQMusic] lyric response empty for" << songmid;
        return {};
    }

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[QQMusic] lyric parse failed" << songmid << err.errorString()
                   << "body=" << QString::fromUtf8(body.left(180));
        return {};
    }

    const QString b64 = doc.object().value("lyric").toString();
    if (b64.isEmpty()) {
        qWarning() << "[QQMusic] lyric field empty for" << songmid;
        return {};
    }

    return QString::fromUtf8(QByteArray::fromBase64(b64.toUtf8()));
}
} // namespace

Config getConfig()
{
    return {QStringLiteral("QQ Music"), QStringLiteral("0.1"), QStringLiteral("ohyeah")};
}

void getLyrics(const lyrics_fetcher::TrackMeta& meta, lyrics_fetcher::LyricsSink& sink,
               QNetworkAccessManager* nam)
{
    qDebug() << "[QQMusic] search start title=" << meta.rawTitle << "artist=" << meta.rawArtist;
    const QVector<SongEntry> songs = searchSongs(meta, nam);
    int added                      = 0;
    for (const SongEntry& song : songs) {
        const QString lyric = queryLyricBySongmid(song.songmid, nam);
        if (lyric.trimmed().isEmpty()) {
            continue;
        }

        auto result      = sink.create_lyric();
        result.title     = song.title;
        result.artist    = song.artist;
        result.album     = song.album;
        result.lyricText = lyric;
        result.source    = QStringLiteral("QQMusic");
        sink.add_lyric(result);
        ++added;
    }
    qDebug() << "[QQMusic] search done, added lyrics=" << added;
}

} // namespace qqmusic_qt6
