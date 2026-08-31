#pragma once

#include <QByteArray>
#include <QString>

namespace lyrics_fetcher
{

bool parse_krc_to_lrc(const QByteArray& krcData, QString& outLrc);
bool parseQrcTextToLrc(const QString& qrcText, QString& outLrc);

} // namespace lyrics_fetcher
