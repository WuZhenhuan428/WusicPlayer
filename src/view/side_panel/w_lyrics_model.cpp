#include "w_lyrics_model.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <algorithm>

#include "core/logger/logger_manager.h"
namespace
{
// NOLINTNEXTLINE
Logger* logger = LoggerManager::file_logger("lyrics_model", {"console", "gui"});
} // namespace

WLyricsModel::WLyricsModel(QObject* parent) : QAbstractListModel(parent) {}

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
    parser_.clear();
    lrc_file_             = {};
    QString default_lines = QStringLiteral("[00:00.00] %1\n[00:01.00] %2").arg(title_artist, album);
    raw_lyrics_text_      = default_lines;
    parser_.parseString(default_lines.toUtf8().toStdString());
    lrc_file_    = parser_.getData();
    current_row_ = -1;
    endResetModel();

    emit sgn_use_timeline_follow(false);
    emit current_line_changed(album, title_artist);
}

bool WLyricsModel::set_raw_lyrics(const QString& raw_data)
{
    beginResetModel();
    parser_.clear();
    lrc_file_    = {};
    current_row_ = -1;
    if (raw_data.isEmpty()) {
        raw_lyrics_text_.clear();
        endResetModel();
        emit current_line_changed(QString(), QString());
        return false;
    }
    if (!parser_.parseString(raw_data.toStdString())) {
        endResetModel();
        emit current_line_changed(QString(), QString());
        return false;
    }
    lrc_file_        = parser_.getData();
    raw_lyrics_text_ = raw_data;
    if (lrc_file_.type == LrcParser::LrcType::WordSync) {
        lrc_file_ = LrcParser::wordToLine(lrc_file_);
    }
    endResetModel();
    emit sgn_use_timeline_follow(true);
    emit current_line_changed(QString(), QString());
    return true;
}

bool WLyricsModel::set_local_lrc(const QString& filepath)
{
    beginResetModel();
    parser_.clear();
    lrc_file_    = {};
    current_row_ = -1;
    if (filepath.isEmpty()) {
        raw_lyrics_text_.clear();
        endResetModel();
        emit current_line_changed(QString(), QString());
        return false;
    }
    QFileInfo audio_fileinfo(filepath);
    if (!audio_fileinfo.exists()) {
        logger->warn("[WARNING] Audio file does not exist: {}", filepath);
        raw_lyrics_text_.clear();
        endResetModel();
        emit current_line_changed(QString(), QString());
        return false;
    }
    QString lrc_path = audio_fileinfo.path() + "/" + audio_fileinfo.completeBaseName() + ".lrc";
    QFileInfo possibel_lrc_fileinfo(lrc_path);
    if (possibel_lrc_fileinfo.exists() && possibel_lrc_fileinfo.isFile()) {
        QFile lrc_file(lrc_path);
        if (lrc_file.open(QIODevice::ReadOnly)) {
            raw_lyrics_text_ = QString::fromUtf8(lrc_file.readAll());
            lrc_file.close();
        }

        parser_.parseFile(lrc_path.toStdString());
        lrc_file_ = parser_.getData();
        if (lrc_file_.type == LrcParser::LrcType::WordSync) {
            lrc_file_ = LrcParser::wordToLine(lrc_file_);
        }
        endResetModel();
        emit sgn_use_timeline_follow(true);
        emit current_line_changed(QString(), QString());
        return true;
    }
    raw_lyrics_text_.clear();
    endResetModel();
    emit current_line_changed(QString(), QString());
    return false;
}

bool WLyricsModel::set_lrc_file_path(const QString& lrc_path)
{
    beginResetModel();
    parser_.clear();
    lrc_file_    = {};
    current_row_ = -1;

    if (lrc_path.isEmpty()) {
        raw_lyrics_text_.clear();
        endResetModel();
        emit current_line_changed(QString(), QString());
        return false;
    }

    QFileInfo lrc_file_info(lrc_path);
    if (!lrc_file_info.exists() || !lrc_file_info.isFile()) {
        raw_lyrics_text_.clear();
        endResetModel();
        emit current_line_changed(QString(), QString());
        return false;
    }

    QFile lrc_file(lrc_path);
    if (lrc_file.open(QIODevice::ReadOnly)) {
        raw_lyrics_text_ = QString::fromUtf8(lrc_file.readAll());
        lrc_file.close();
    }

    if (!parser_.parseFile(lrc_path.toStdString())) {
        raw_lyrics_text_.clear();
        endResetModel();
        emit current_line_changed(QString(), QString());
        return false;
    }

    lrc_file_ = parser_.getData();
    if (lrc_file_.type == LrcParser::LrcType::WordSync) {
        lrc_file_ = LrcParser::wordToLine(lrc_file_);
    }

    endResetModel();
    emit sgn_use_timeline_follow(true);
    emit current_line_changed(QString(), QString());
    return true;
}

QString WLyricsModel::raw_lyrics_text() const
{
    return raw_lyrics_text_;
}

int WLyricsModel::get_row_by_position(qint64 pos_ms)
{
    if (lrc_file_.lyrics.empty()) {
        return -1;
    }
    // dichotomy
    auto it = std::ranges::upper_bound(
        lrc_file_.lyrics, pos_ms, std::less<>{},
        [](const LrcParser::LrcUnit& unit) { return static_cast<qint64>(unit.time_ms); });
    if (it == lrc_file_.lyrics.begin()) {
        return 0;
    }
    return static_cast<int>(std::distance(lrc_file_.lyrics.begin(), it) - 1);
}

int WLyricsModel::current_row() const
{
    return current_row_;
}

QModelIndex WLyricsModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent)) {
        return {};
    }
    if (row < 0 || row >= rowCount(parent)) {
        return {};
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
    return static_cast<int>(lrc_file_.lyrics.size());
}

QVariant WLyricsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return {};
    }
    const int row         = index.row();
    const int lyrics_size = static_cast<int>(lrc_file_.lyrics.size());
    if (row < 0 || row >= lyrics_size) {
        return {};
    }

    const auto& unit = lrc_file_.lyrics.at(row);

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
        return row == current_row_;
    }
    return {};
}

void WLyricsModel::set_current_position(qint64 pos_ms)
{
    const int new_row = get_row_by_position(pos_ms);
    if (new_row == current_row_) {
        return;
    }

    const int old_row = current_row_;
    current_row_      = new_row;

    if (old_row >= 0 && old_row < rowCount()) {
        const QModelIndex idx = index(old_row, 0);
        emit dataChanged(idx, idx, {UserDefineRole::CurrentLine});
    }
    if (current_row_ >= 0 && current_row_ < rowCount()) {
        const QModelIndex idx = index(current_row_, 0);
        emit dataChanged(idx, idx, {UserDefineRole::CurrentLine});
        emit current_line_changed(current_line_text(), next_line_text());
    } else {
        emit current_line_changed(QString(), QString());
    }
}

QString WLyricsModel::prev_line_text() const
{
    if (current_row_ < 1 || current_row_ >= rowCount() + 1) {
        return {};
    }
    return data(index(current_row_ - 1, 0), Qt::DisplayRole).toString();
}

QString WLyricsModel::current_line_text() const
{
    if (current_row_ < 0 || current_row_ >= rowCount()) {
        return {};
    }
    return data(index(current_row_, 0), Qt::DisplayRole).toString();
}

QString WLyricsModel::next_line_text() const
{
    if (current_row_ < -1 || current_row_ >= rowCount() - 1) {
        return {};
    }
    return data(index(current_row_ + 1, 0), Qt::DisplayRole).toString();
}
