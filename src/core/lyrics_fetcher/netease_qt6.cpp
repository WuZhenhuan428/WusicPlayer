#include "netease_qt6.h"

#include <QByteArray>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>
#include <openssl/evp.h>

namespace netease_qt6
{

struct SongCandidate
{
    qint64 id = 0;
    QString title;
    QString artist;
    QString album;
};

static const QByteArray kLinuxApiKey("rFgB&h#%2?^eDg:Q");
static const QString kAnonymousToken = "bf8bfeabb1aa84f9c8c3906c04a04fb864322804c83f5d607e91a04eae4"
                                       "63c9436bd1a17ec353cf780b396507a3f7464"
                                       "e8a60f4bbc019437993166e004087dd32d1490298caf655c2353e58daa0"
                                       "bc13cc7d5c198250968580b12c1b8817e3f5c"
                                       "807e650dd04abd3fb8130b7ae43fcc5b";

static QByteArray aesEncryptEcb(const QByteArray& plain, const QByteArray& key)
{
    QByteArray out(plain.size() + EVP_MAX_BLOCK_LENGTH, Qt::Uninitialized);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return {};
    }

    int outLen1 = 0;
    int outLen2 = 0;

    bool ok =
        EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), nullptr,
                           reinterpret_cast<const unsigned char*>(key.constData()), nullptr) == 1;
    if (ok) {
        ok = EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char*>(out.data()), &outLen1,
                               reinterpret_cast<const unsigned char*>(plain.constData()),
                               plain.size()) == 1;
    }
    if (ok) {
        ok = EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(out.data() + outLen1),
                                 &outLen2) == 1;
    }

    EVP_CIPHER_CTX_free(ctx);

    if (!ok) {
        return {};
    }

    out.resize(outLen1 + outLen2);
    return out;
}

static QString procKeywords(QString s)
{
    s = s.toLower();
    s.remove(QRegularExpression("['·$&–]"));
    s.remove(QRegularExpression("\\(.*?\\)|\\[.*?\\]|\\{.*?\\}|（.*?"));
    s.remove(QRegularExpression("[-/:-@[-`{-~]+"));
    s.remove(
        QRegularExpression("[\\u2014\\u2018\\u201c\\u2026\\u3001\\u3002\\u300a\\u300b\\u300e\\u300f"
                           "\\u3010\\u3011\\u30fb\\uff01\\uff08\\uff09\\uff0c\\uff1a\\uff1b\\uff1f"
                           "\\uff5e\\uffe5]+"));
    return s;
}

static QMap<QString, QString> linuxApiPayload(const QString& method, const QString& url,
                                              const QJsonObject& params)
{
    QJsonObject wrapped;
    wrapped.insert("method", method);
    wrapped.insert("url", url);
    wrapped.insert("params", params);

    const QByteArray text      = QJsonDocument(wrapped).toJson(QJsonDocument::Compact);
    const QByteArray encrypted = aesEncryptEcb(text, kLinuxApiKey);
    const QString eparams      = QString::fromLatin1(encrypted.toHex()).toUpper();

    QMap<QString, QString> data;
    data.insert("eparams", eparams);
    return data;
}

static QByteArray doRequest(const QString& method, const QUrl& originUrl, const QJsonObject& data,
                            const QString& crypto, QNetworkAccessManager* nam)
{
    QUrl url = originUrl;
    QMap<QString, QString> form;

    QNetworkRequest req;
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    if (url.host().contains("music.163.com")) {
        req.setRawHeader("Referer", "https://music.163.com");
    }

    if (crypto == "linuxapi") {
        QString apiUrl = url.toString();
        apiUrl.replace(QRegularExpression("\\w*api"), "api");

        form = linuxApiPayload(method, apiUrl, data);
        req.setRawHeader("User-Agent",
                         "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) "
                         "Chrome/60.0.3112.90 Safari/537.36");
        req.setRawHeader("Cookie", QString("MUSIC_A=%1").arg(kAnonymousToken).toUtf8());
        url = QUrl("https://music.163.com/api/linux/forward");
    } else {
        return {};
    }

    req.setUrl(url);

    QUrlQuery query;
    for (auto it = form.cbegin(); it != form.cend(); ++it) {
        query.addQueryItem(it.key(), it.value());
    }
    const QByteArray body = query.toString(QUrl::FullyEncoded).toUtf8();

    QNetworkReply* reply  = nullptr;
    if (method.compare("POST", Qt::CaseInsensitive) == 0) {
        reply = nam->post(req, body);
    } else {
        reply = nam->get(req);
    }

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const bool ok         = reply->error() == QNetworkReply::NoError;
    const int code        = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QByteArray resp = reply->readAll();
    reply->deleteLater();

    if (!ok || code != 200) {
        return {};
    }
    return resp;
}

static QList<SongCandidate> parseSearchResults(const QByteArray& body)
{
    QList<SongCandidate> candidates;
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return candidates;
    }

    const QJsonObject root   = doc.object();
    const QJsonObject result = root.value("result").toObject();
    const QJsonArray songs   = result.value("songs").toArray();

    for (const QJsonValue& v : songs) {
        const QJsonObject song = v.toObject();
        if (!song.contains("id") || !song.contains("name")) {
            continue;
        }

        SongCandidate c;
        c.id                     = static_cast<qint64>(song.value("id").toDouble());
        c.title                  = song.value("name").toString();

        const QJsonArray artists = song.value("artists").toArray();
        for (const QJsonValue& av : artists) {
            const QJsonObject ao = av.toObject();
            if (ao.contains("name")) {
                c.artist = ao.value("name").toString();
                break;
            }
        }

        c.album = song.value("album").toObject().value("name").toString();
        candidates.push_back(c);
    }

    return candidates;
}

static void parseLyricResponse(const SongCandidate& item, LyricsSink& sink, const QByteArray& body)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return;
    }

    const QJsonObject lyricObj = doc.object();
    QString lyricText;

    if (lyricObj.contains("lrc")) {
        const QJsonObject lrc = lyricObj.value("lrc").toObject();
        lyricText             = lrc.value("lyric").toString();
        const int version     = lrc.value("version").toInt();
        if (version == 1) {
            return;
        }
    }

    if (lyricObj.contains("tlyric")) {
        lyricText += lyricObj.value("tlyric").toObject().value("lyric").toString();
    }

    LyricMeta meta = sink.create_lyric();
    meta.title     = item.title;
    meta.artist    = item.artist;
    meta.album     = item.album;
    meta.lyricText = lyricText;
    meta.source    = QStringLiteral("Netease");
    sink.add_lyric(meta);
}

Config getConfig()
{
    return {QStringLiteral("网易云音乐"), QStringLiteral("0.3"), QStringLiteral("ohyeah")};
}

void getLyrics(const TrackMeta& meta, LyricsSink& sink, QNetworkAccessManager* nam)
{
    if (!nam) {
        return;
    }

    const QString title  = procKeywords(meta.rawTitle);
    const QString artist = procKeywords(meta.rawArtist);

    QJsonObject searchData;
    searchData.insert("s", QString("%1 %2").arg(title, artist));
    searchData.insert("type", 1);
    searchData.insert("limit", 10);
    searchData.insert("offset", 0);

    const QByteArray searchBody = doRequest(
        QStringLiteral("POST"), QUrl(QStringLiteral("https://music.163.com/weapi/search/get")),
        searchData, QStringLiteral("linuxapi"), nam);

    if (searchBody.isEmpty()) {
        return;
    }

    const QList<SongCandidate> candidates = parseSearchResults(searchBody);
    for (const SongCandidate& item : candidates) {
        QJsonObject queryData;
        queryData.insert("id", QString::number(item.id));

        const QByteArray lyricBody = doRequest(
            QStringLiteral("POST"),
            QUrl(QStringLiteral("https://music.163.com/weapi/song/lyric?lv=-1&kv=-1&tv=-1")),
            queryData, QStringLiteral("linuxapi"), nam);

        parseLyricResponse(item, sink, lyricBody);
    }
}

} // namespace netease_qt6
