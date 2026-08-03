#include "krc_qrc_parser.h"

#include <QRegularExpression>
#include <QXmlStreamReader>
#include <zlib.h>

namespace lyrics_fetcher {

namespace {
QString format_time(qint64 ms) {
    if (ms < 0) {
        ms = 0;
    }
    const qint64 totalSec = ms / 1000;
    const qint64 min = totalSec / 60;
    const qint64 sec = totalSec % 60;
    const qint64 cent = (ms % 1000) / 10;
    return QString("%1:%2.%3")
        .arg(min, 2, 10, QChar('0'))
        .arg(sec, 2, 10, QChar('0'))
        .arg(cent, 2, 10, QChar('0'));
}

bool inflateZlib(const QByteArray& compressed, QByteArray& out) {
    z_stream zs{};
    zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(compressed.constData()));
    zs.avail_in = static_cast<uInt>(compressed.size());

    if (inflateInit(&zs) != Z_OK) {
        return false;
    }

    QByteArray buffer;
    buffer.resize(64 * 1024);

    int ret = Z_OK;
    while (ret == Z_OK) {
        zs.next_out = reinterpret_cast<Bytef*>(buffer.data());
        zs.avail_out = static_cast<uInt>(buffer.size());
        ret = inflate(&zs, Z_NO_FLUSH);
        if (ret != Z_OK && ret != Z_STREAM_END) {
            inflateEnd(&zs);
            return false;
        }

        const int produced = static_cast<int>(buffer.size() - zs.avail_out);
        if (produced > 0) {
            out.append(buffer.constData(), produced);
        }
    }

    inflateEnd(&zs);
    return ret == Z_STREAM_END;
}
}

bool parse_krc_to_lrc(const QByteArray& krcData, QString& outLrc) {
    outLrc.clear();
    static const QByteArray magic("krc1", 4);
    static const unsigned char key[] = {
        0x40, 0x47, 0x61, 0x77, 0x5e, 0x32, 0x74, 0x47,
        0x51, 0x36, 0x31, 0x2d, 0xce, 0xd2, 0x6e, 0x69
    };

    if (!krcData.startsWith(magic) || krcData.size() <= magic.size()) {
        return false;
    }

    QByteArray xored;
    xored.resize(krcData.size() - magic.size());
    for (int i = magic.size(); i < krcData.size(); ++i) {
        const int keyIndex = (i - magic.size()) % static_cast<int>(sizeof(key));
        xored[i - magic.size()] = static_cast<char>(static_cast<unsigned char>(krcData.at(i)) ^ key[keyIndex]);
    }

    QByteArray unzipped;
    if (!inflateZlib(xored, unzipped)) {
        return false;
    }

    const QString text = QString::fromUtf8(unzipped);
    QStringList output;

    const QRegularExpression lineRe(R"(^\[(\d+),(\d+)\](.*)$)");
    const QRegularExpression unitRe(R"(<(\d+),(\d+),(\d+)>([^<]*))");

    const QStringList lines = text.split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        QRegularExpressionMatch m = lineRe.match(line);
        if (!m.hasMatch()) {
            continue;
        }

        const qint64 base = m.captured(1).toLongLong();
        const qint64 duration = m.captured(2).toLongLong();
        const QString payload = m.captured(3);

        QString lrc = QString("[%1]").arg(format_time(base));
        QRegularExpressionMatchIterator it = unitRe.globalMatch(payload);
        while (it.hasNext()) {
            const QRegularExpressionMatch um = it.next();
            const qint64 offset = um.captured(1).toLongLong();
            const QString word = um.captured(4);
            lrc += QString("<%1>%2").arg(format_time(base + offset), word);
        }
        lrc += QString("<%1>").arg(format_time(base + duration));
        output << lrc;
    }

    outLrc = output.join("\n");
    return !outLrc.trimmed().isEmpty();
}

bool parseQrcTextToLrc(const QString& qrcText, QString& outLrc) {
    outLrc.clear();

    if (!qrcText.contains("<?xml")) {
        outLrc = qrcText;
        return !outLrc.trimmed().isEmpty();
    }

    QXmlStreamReader xml(qrcText);
    QString lyricContent;
    while (!xml.atEnd()) {
        xml.readNext();
        if (xml.isStartElement() && xml.name().toString() == "Lyric_1") {
            lyricContent = xml.attributes().value("LyricContent").toString();
            break;
        }
    }

    if (lyricContent.isEmpty()) {
        return false;
    }

    QString text = lyricContent;
    text.replace(QRegularExpression(R"(^\[(\d+),(\d+)\])", QRegularExpression::MultilineOption),
                 "[$1]<$1>");
    text.replace(QRegularExpression(R"(\((\d+),(\d+)\))"), "<$1>");
    outLrc = text;
    return !outLrc.trimmed().isEmpty();
}

} // namespace lyrics_fetcher
