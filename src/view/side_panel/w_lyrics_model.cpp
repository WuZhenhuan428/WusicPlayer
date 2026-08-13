#include "w_lyrics_model.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <algorithm>

#include "core/logger/logger_manager.h"
namespace
{
Logger* logger = LoggerManager::file_logger("lyrics_model", {"console", "gui"});
}

WLyricsModel::WLyricsModel(QObject* parent) : QAbstractListModel(parent) {}

WLyricsModel::~WLyricsModel() {}

void WLyricsModel::set_default_info(const TrackMetaData& meta)
{
    const QString title =
        meta.title.trimmed().isEmpty() ? QStringLiteral("Unknown Title") : meta.title.trimmed();
    const QString artist =
        meta.artist.trimmed().isEmpty() ? QStringLiteral("Unknown Artist") : meta.artist.trimmed();
    const QString album =
        meta.album.trimmed().isEmpty() ? QStringLiteral("Unknown Album") : meta.album.trimmed();
    const QString title_artist = title + QStringLiteral(" - ") + artist;

    beginResetModel();
    m_parser.clear();
    m_lrc_file            = {};
    QString default_lines = QStringLiteral("[00:00.00] %1\n[00:01.00] %2").arg(title_artist, album);
    m_raw_lyrics_text     = default_lines;
    m_parser.parseString(default_lines.toUtf8().toStdString());
    m_lrc_file    = m_parser.getData();
    m_current_row = -1;
    endResetModel();

    emit sgnUseTimelineFollow(false);
    emit currentLineChanged(album, title_artist);
}

bool WLyricsModel::set_raw_lyrics(const QString& raw_data)
{
    beginResetModel();
    m_parser.clear();
    m_lrc_file    = {};
    m_current_row = -1;
    if (raw_data.isEmpty()) {
        m_raw_lyrics_text.clear();
        endResetModel();
        emit currentLineChanged(QString(), QString());
        return false;
    }
    if (!m_parser.parseString(raw_data.toStdString())) {
        endResetModel();
        emit currentLineChanged(QString(), QString());
        return false;
    }
    m_lrc_file        = m_parser.getData();
    m_raw_lyrics_text = raw_data;
    if (m_lrc_file.type == LrcParser::LrcType::WordSync) {
        m_lrc_file = LrcParser::wordToLine(m_lrc_file);
    }
    endResetModel();
    emit sgnUseTimelineFollow(true);
    emit currentLineChanged(QString(), QString());
    return true;
}

bool WLyricsModel::set_local_lrc(const QString& filepath)
{
    beginResetModel();
    m_parser.clear();
    m_lrc_file    = {};
    m_current_row = -1;
    if (filepath.isEmpty()) {
        m_raw_lyrics_text.clear();
        endResetModel();
        emit currentLineChanged(QString(), QString());
        return false;
    }
    QFileInfo audio_fileinfo(filepath);
    if (!audio_fileinfo.exists()) {
        logger->warn("[WARNING] Audio file does not exist: {}", filepath);
        m_raw_lyrics_text.clear();
        endResetModel();
        emit currentLineChanged(QString(), QString());
        return false;
    }
    QString lrc_path = audio_fileinfo.path() + "/" + audio_fileinfo.completeBaseName() + ".lrc";
    QFileInfo possibel_lrc_fileinfo(lrc_path);
    if (possibel_lrc_fileinfo.exists() && possibel_lrc_fileinfo.isFile()) {
        QFile lrc_file(lrc_path);
        if (lrc_file.open(QIODevice::ReadOnly)) {
            m_raw_lyrics_text = QString::fromUtf8(lrc_file.readAll());
            lrc_file.close();
        }

        m_parser.parseFile(lrc_path.toStdString());
        m_lrc_file = m_parser.getData();
        if (m_lrc_file.type == LrcParser::LrcType::WordSync) {
            m_lrc_file = LrcParser::wordToLine(m_lrc_file);
        }
        endResetModel();
        emit sgnUseTimelineFollow(true);
        emit currentLineChanged(QString(), QString());
        return true;
    }
    m_raw_lyrics_text.clear();
    endResetModel();
    emit currentLineChanged(QString(), QString());
    return false;
}

bool WLyricsModel::set_lrc_file_path(const QString& lrc_path)
{
    beginResetModel();
    m_parser.clear();
    m_lrc_file    = {};
    m_current_row = -1;

    if (lrc_path.isEmpty()) {
        m_raw_lyrics_text.clear();
        endResetModel();
        emit currentLineChanged(QString(), QString());
        return false;
    }

    QFileInfo lrc_file_info(lrc_path);
    if (!lrc_file_info.exists() || !lrc_file_info.isFile()) {
        m_raw_lyrics_text.clear();
        endResetModel();
        emit currentLineChanged(QString(), QString());
        return false;
    }

    QFile lrc_file(lrc_path);
    if (lrc_file.open(QIODevice::ReadOnly)) {
        m_raw_lyrics_text = QString::fromUtf8(lrc_file.readAll());
        lrc_file.close();
    }

    if (!m_parser.parseFile(lrc_path.toStdString())) {
        m_raw_lyrics_text.clear();
        endResetModel();
        emit currentLineChanged(QString(), QString());
        return false;
    }

    m_lrc_file = m_parser.getData();
    if (m_lrc_file.type == LrcParser::LrcType::WordSync) {
        m_lrc_file = LrcParser::wordToLine(m_lrc_file);
    }

    endResetModel();
    emit sgnUseTimelineFollow(true);
    emit currentLineChanged(QString(), QString());
    return true;
}

QString WLyricsModel::raw_lyrics_text() const
{
    return m_raw_lyrics_text;
}

int WLyricsModel::get_row_by_position(qint64 pos_ms)
{
    if (m_lrc_file.lyrics.empty()) {
        return -1;
    }
    // dichotomy
    auto it = std::upper_bound(m_lrc_file.lyrics.begin(), m_lrc_file.lyrics.end(), pos_ms,
                               [](qint64 ms, const LrcParser::LrcUnit& unit) {
                                   return ms < static_cast<qint64>(unit.time_ms);
                               });
    if (it == m_lrc_file.lyrics.begin()) {
        return 0;
    }
    return std::distance(m_lrc_file.lyrics.begin(), it) - 1;
}

int WLyricsModel::current_row() const
{
    return m_current_row;
}

QModelIndex WLyricsModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }
    if (row < 0 || row >= static_cast<int>(m_lrc_file.lyrics.size())) {
        return QModelIndex();
    }
    return createIndex(row, column);
}

int WLyricsModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return 1;
}

int WLyricsModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_lrc_file.lyrics.size();
}

QVariant WLyricsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    int row = index.row();
    if (row < 0 || row >= static_cast<int>(m_lrc_file.lyrics.size())) {
        return QVariant();
    }

    const auto& unit = m_lrc_file.lyrics.at(row);

    if (role == Qt::DisplayRole) {
        return QString::fromStdString(unit.text);
    }
    if (role == UserDefineRole::CurrentTime) { // user defined: TimeRole
        return static_cast<qlonglong>(unit.time_ms);
    }

    if (role == Qt::TextAlignmentRole) {
        return Qt::AlignCenter;
    }

    if (role == UserDefineRole::CurrentLine) {
        return row == m_current_row;
    }
    return QVariant();
}

void WLyricsModel::set_current_position(qint64 pos_ms)
{
    const int new_row = get_row_by_position(pos_ms);
    if (new_row == m_current_row)
        return;

    const int old_row = m_current_row;
    m_current_row     = new_row;

    if (old_row >= 0 && old_row < rowCount()) {
        const QModelIndex idx = index(old_row, 0);
        emit dataChanged(idx, idx, {UserDefineRole::CurrentLine});
    }
    if (m_current_row >= 0 && m_current_row < rowCount()) {
        const QModelIndex idx = index(m_current_row, 0);
        emit dataChanged(idx, idx, {UserDefineRole::CurrentLine});
        emit currentLineChanged(current_line_text(), next_line_text());
    } else {
        emit currentLineChanged(QString(), QString());
    }
}

QString WLyricsModel::prev_line_text() const
{
    if (m_current_row < 1 || m_current_row >= rowCount() + 1)
        return QString();
    return data(index(m_current_row - 1, 0), Qt::DisplayRole).toString();
}

QString WLyricsModel::current_line_text() const
{
    if (m_current_row < 0 || m_current_row >= rowCount())
        return QString();
    return data(index(m_current_row, 0), Qt::DisplayRole).toString();
}

QString WLyricsModel::next_line_text() const
{
    if (m_current_row < -1 || m_current_row >= rowCount() - 1)
        return QString();
    return data(index(m_current_row + 1, 0), Qt::DisplayRole).toString();
}
