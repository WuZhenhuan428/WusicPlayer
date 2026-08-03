#include "kugou_qt6.h"

#include "krc_qrc_parser.h"

#include <QByteArray>
#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <algorithm>

namespace kugou_qt6 {

namespace {
struct Candidate {
    QString id;
    QString accessKey;
    QString title;
    QString artist;
};

QByteArray doGet(const QUrl& url, QNetworkAccessManager* nam) {
    if (!nam) {
        return {};
    }

    QNetworkRequest req(url);
    req.setRawHeader("User-Agent", "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120 Safari/537.36");
    QNetworkReply* reply = nam->get(req);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const bool ok = reply->error() == QNetworkReply::NoError;
    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QString errText = reply->errorString();
    const QByteArray body = reply->readAll();
    reply->deleteLater();
    if (!ok || status != 200) {
        qWarning() << "[Kugou] GET failed" << url << "status=" << status << "error=" << errText;
        return {};
    }
    qDebug() << "[Kugou] GET ok" << url << "bytes=" << body.size();
    return body;
}

QVector<Candidate> searchCandidates(const lyrics_fetcher::TrackMeta& meta, QNetworkAccessManager* nam) {
    QVector<Candidate> out;

    const QString title = meta.rawTitle.trimmed();
    const QString artist = meta.rawArtist.trimmed();
    const QString keyword = artist.isEmpty() ? title : (artist + "-" + title);
    if (keyword.isEmpty()) {
        qWarning() << "[Kugou] empty keyword";
        return out;
    }

    QUrlQuery q;
    q.addQueryItem("ver", "1");
    q.addQueryItem("man", "yes");
    q.addQueryItem("client", "pc");
    q.addQueryItem("keyword", keyword);
    const int durationMs = std::max(0, meta.durationSec) * 1000;
    if (durationMs > 0) {
        q.addQueryItem("duration", QString::number(durationMs));
    }
    q.addQueryItem("hash", "");

    QUrl url("http://lyrics.kugou.com/search");
    url.setQuery(q);

    const QByteArray body = doGet(url, nam);
    if (body.isEmpty()) {
        qWarning() << "[Kugou] search body empty title=" << meta.rawTitle << "artist=" << meta.rawArtist << "durationSec=" << meta.durationSec;
        return out;
    }

    qDebug() << "[Kugou] keyword=" << keyword;

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[Kugou] search parse failed" << err.errorString() << "body=" << QString::fromUtf8(body.left(180));
        return out;
    }

    const QJsonArray list = doc.object().value("candidates").toArray();
    for (const auto& item : list) {
        const QJsonObject o = item.toObject();
        Candidate c;
        c.id = QString::number(static_cast<qint64>(o.value("id").toDouble()));
        c.accessKey = o.value("accesskey").toString();
        c.title = o.value("song").toString();
        c.artist = o.value("singer").toString();
        if (!c.id.isEmpty() && !c.accessKey.isEmpty()) {
            out.push_back(c);
        }
    }

    qDebug() << "[Kugou] search candidates=" << out.size() << "durationSec=" << meta.durationSec;

    return out;
}

QString downloadAndParse(const Candidate& c, QNetworkAccessManager* nam) {
    QUrlQuery q;
    q.addQueryItem("ver", "1");
    q.addQueryItem("client", "pc");
    q.addQueryItem("id", c.id);
    q.addQueryItem("accesskey", c.accessKey);
    q.addQueryItem("fmt", "krc");
    q.addQueryItem("charset", "utf8");

    QUrl url("http://lyrics.kugou.com/download");
    url.setQuery(q);

    const QByteArray body = doGet(url, nam);
    if (body.isEmpty()) {
        qWarning() << "[Kugou] lyric body empty id=" << c.id;
        return {};
    }

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(body, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "[Kugou] lyric parse failed id=" << c.id << err.errorString() << "body=" << QString::fromUtf8(body.left(180));
        return {};
    }

    const QString b64 = doc.object().value("content").toString();
    if (b64.isEmpty()) {
        qWarning() << "[Kugou] empty content id=" << c.id;
        return {};
    }

    const QByteArray krcData = QByteArray::fromBase64(b64.toUtf8());
    QString lrc;
    if (!lyrics_fetcher::parse_krc_to_lrc(krcData, lrc)) {
        qWarning() << "[Kugou] failed to parse krc id=" << c.id << "bytes=" << krcData.size();
        return {};
    }

    return lrc;
}
}

Config getConfig() {
    return {QStringLiteral("Kugou Music"), QStringLiteral("0.1"), QStringLiteral("anonymous")};
}

void getLyrics(const lyrics_fetcher::TrackMeta& meta,
               lyrics_fetcher::LyricsSink& sink,
               QNetworkAccessManager* nam) {
    qDebug() << "[Kugou] search start title=" << meta.rawTitle << "artist=" << meta.rawArtist << "durationSec=" << meta.durationSec;

    QVector<Candidate> candidates = searchCandidates(meta, nam);
    if (candidates.isEmpty() && meta.durationSec > 0) {
        lyrics_fetcher::TrackMeta fallback = meta;
        fallback.durationSec = 0;
        qDebug() << "[Kugou] retry search without duration";
        candidates = searchCandidates(fallback, nam);
    }

    int added = 0;
    for (const Candidate& c : candidates) {
        const QString lyric = downloadAndParse(c, nam);
        if (lyric.trimmed().isEmpty()) {
            continue;
        }

        auto result = sink.create_lyric();
        result.title = c.title;
        result.artist = c.artist;
        result.album.clear();
        result.lyricText = lyric;
        result.source = QStringLiteral("Kugou");
        sink.add_lyric(result);
        ++added;
    }

    qDebug() << "[Kugou] search done, added lyrics=" << added;
}

} // namespace kugou_qt6
