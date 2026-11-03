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

namespace
{
struct SongCandidate
{
    qint64 id = 0;
    QString title;
    QString artist;
    QString album;
};
} // namespace

namespace netease_qt6
{
using namespace lyrics_fetcher;

// NOLINT for static values
// NOLINTBEGIN
static const QByteArray kLinuxApiKey("rFgB&h#%2?^eDg:Q");
static const QString kAnonymousToken = "bf8bfeabb1aa84f9c8c3906c04a04fb864322804c83f5d607e91a04eae4"
                                       "63c9436bd1a17ec353cf780b396507a3f7464"
                                       "e8a60f4bbc019437993166e004087dd32d1490298caf655c2353e58daa0"
                                       "bc13cc7d5c198250968580b12c1b8817e3f5c"
                                       "807e650dd04abd3fb8130b7ae43fcc5b";
// NOLINTEND
static QByteArray aes_encrypt_ecb(const QByteArray& plain, const QByteArray& key)
{
    QByteArray out(plain.size() + EVP_MAX_BLOCK_LENGTH, Qt::Uninitialized);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return {};
    }

    int out_len_1 = 0;
    int out_len_2 = 0;
    bool ok =
        EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), nullptr,
                           reinterpret_cast<const unsigned char*>(key.constData()), nullptr) == 1;
    if (ok) {
        ok = EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char*>(out.data()), &out_len_1,
                               reinterpret_cast<const unsigned char*>(plain.constData()),
                               static_cast<int>(plain.size())) == 1;
    }
    if (ok) {
        ok = EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(out.data() + out_len_1),
                                 &out_len_2) == 1;
    }

    EVP_CIPHER_CTX_free(ctx);

    if (!ok) {
        return {};
    }

    out.resize(out_len_1 + out_len_2);
    return out;
}

static QString proc_keywords(QString s)
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

static QMap<QString, QString> linux_api_payload(const QString& method, const QString& url,
                                                const QJsonObject& params)
{
    QJsonObject wrapped;
    wrapped.insert("method", method);
    wrapped.insert("url", url);
    wrapped.insert("params", params);

    const QByteArray text      = QJsonDocument(wrapped).toJson(QJsonDocument::Compact);
    const QByteArray encrypted = aes_encrypt_ecb(text, kLinuxApiKey);
    const QString eparams      = QString::fromLatin1(encrypted.toHex()).toUpper();

    QMap<QString, QString> data;
    data.insert("eparams", eparams);
    return data;
}

static QByteArray do_request(const QString& method, const QUrl& origin_url, const QJsonObject& data,
                             const QString& crypto, QNetworkAccessManager* nam)
{
    QUrl url = origin_url;
    QMap<QString, QString> form;

    QNetworkRequest req;
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    if (url.host().contains("music.163.com")) {
        req.setRawHeader("Referer", "https://music.163.com");
    }

    if (crypto == "linuxapi") {
        QString api_url = url.toString();
        api_url.replace(QRegularExpression("\\w*api"), "api");

        form = linux_api_payload(method, api_url, data);
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

    const bool ok   = reply->error() == QNetworkReply::NoError;
    const int code  = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QByteArray resp = reply->readAll();
    reply->deleteLater();

    if (!ok || code != 200) {
        return {};
    }
    return resp;
}

static QList<SongCandidate> parse_search_results(const QByteArray& body)
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

    for (const auto& v : songs) {
        const QJsonObject song = v.toObject();
        if (!song.contains("id") || !song.contains("name")) {
            continue;
        }

        SongCandidate c;
        c.id                     = static_cast<qint64>(song.value("id").toDouble());
        c.title                  = song.value("name").toString();

        const QJsonArray artists = song.value("artists").toArray();
        for (const auto& av : artists) {
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

static void parse_lyric_response(const SongCandidate& item, LyricsSink& sink,
                                 const QByteArray& body)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return;
    }

    const QJsonObject lyric_obj = doc.object();
    QString lyric_text;

    if (lyric_obj.contains("lrc")) {
        const QJsonObject lrc = lyric_obj.value("lrc").toObject();
        lyric_text            = lrc.value("lyric").toString();
        const int version     = lrc.value("version").toInt();
        if (version == 1) {
            return;
        }
    }

    if (lyric_obj.contains("tlyric")) {
        lyric_text += lyric_obj.value("tlyric").toObject().value("lyric").toString();
    }

    LyricMeta meta  = sink.create_lyric();
    meta.title      = item.title;
    meta.artist     = item.artist;
    meta.album      = item.album;
    meta.lyric_text = std::move(lyric_text);
    meta.source     = QStringLiteral("Netease");
    sink.add_lyric(meta);
}

Config get_config()
{
    return {QStringLiteral("网易云音乐"), QStringLiteral("0.3"), QStringLiteral("ohyeah")};
}

void get_lyrics(const TrackMeta& meta, LyricsSink& sink, QNetworkAccessManager* nam)
{
    if (!nam) {
        return;
    }

    const QString title  = proc_keywords(meta.raw_title);
    const QString artist = proc_keywords(meta.raw_artist);

    QJsonObject search_data;
    search_data.insert("s", QString("%1 %2").arg(title, artist));
    search_data.insert("type", 1);
    search_data.insert("limit", 10);
    search_data.insert("offset", 0);

    const QByteArray search_body = do_request(
        QStringLiteral("POST"), QUrl(QStringLiteral("https://music.163.com/weapi/search/get")),
        search_data, QStringLiteral("linuxapi"), nam);

    if (search_body.isEmpty()) {
        return;
    }

    const QList<SongCandidate> candidates = parse_search_results(search_body);
    for (const SongCandidate& item : candidates) {
        QJsonObject query_data;
        query_data.insert("id", QString::number(item.id));

        const QByteArray lyric_body = do_request(
            QStringLiteral("POST"),
            QUrl(QStringLiteral("https://music.163.com/weapi/song/lyric?lv=-1&kv=-1&tv=-1")),
            query_data, QStringLiteral("linuxapi"), nam);

        parse_lyric_response(item, sink, lyric_body);
    }
}

} // namespace netease_qt6
