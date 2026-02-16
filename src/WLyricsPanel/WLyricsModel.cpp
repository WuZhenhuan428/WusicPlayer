#include "WLyricsModel.h"

#include <QFileInfo>
#include <QDir>
#include <algorithm>

WLyricsModel::WLyricsModel(QObject* parent)
    : QAbstractListModel(parent)
{

}

WLyricsModel::~WLyricsModel() {}

void WLyricsModel::setDefaultInfo(const QString& filename, const QString& artist) {
    beginResetModel();
    m_parser.clear();
    QString foo;
    foo.append("[00:00.000] Title: ");
    foo.append(filename);
    foo.append("\n[00:00.001] Artist: ");
    foo.append(artist);
    foo.append("\n");
    auto bar = foo.toUtf8();
    m_parser.parseString(bar.toStdString());
    endResetModel();
}

bool WLyricsModel::setRawLyrics(const QString& raw_data) {
    beginResetModel();
    m_parser.clear();
    if (raw_data.isEmpty()) {
        return false;
    }
    if (m_parser.parseString(raw_data.toStdString())) {
        qDebug() << "[LRC] Loaded from cache metadata";
    }
    endResetModel();
    return true;
}

bool WLyricsModel::setLocalLrc(const QString& filepath) {
    beginResetModel();
    m_parser.clear();
    if (filepath.isEmpty()) {
        endResetModel();
        return false;
    }
    QFileInfo audio_fileinfo(filepath);
    if (!audio_fileinfo.exists()) {
        qDebug() << "[WARNING] Audio file does not exist: " << filepath;
        endResetModel();
        return false;
    }
    QString lrc_path = audio_fileinfo.path() + "/" + audio_fileinfo.completeBaseName() + ".lrc";
    QFileInfo possibel_lrc_fileinfo(lrc_path);
    if (possibel_lrc_fileinfo.exists() && possibel_lrc_fileinfo.isFile()) {
        m_parser.parseFile(lrc_path.toStdString());
        endResetModel();
        return true;
    }
    endResetModel();
    return false;
}

int WLyricsModel::getCurrentRow(qint64 pos_ms) {
    const auto& lyrics = m_parser.getData().lyrics;
    if (lyrics.empty()) {
        return -1;
    }
    // dichotomy
    auto it = std::upper_bound(lyrics.begin(), lyrics.end(), pos_ms,
        [](qint64 ms, const LrcUnit& unit){
            return ms < unit.time_ms;
        }
    );
    if (it == lyrics.begin()) {
        return 0;
    }
    return std::distance(lyrics.begin(), it) - 1;
}


QModelIndex WLyricsModel::index(int row, int column, const QModelIndex &parent) const {
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }
    if (row < 0 || row >= static_cast<int>(m_parser.getUnitCount())) {
        return QModelIndex();
    }
    return createIndex(row, column);
}

int WLyricsModel::columnCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return 1;
}

int WLyricsModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return m_parser.getUnitCount();
}

QVariant WLyricsModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) {
        return QVariant();
    }
    int row = index.row();
    if (row < 0 || row >= static_cast<int>(m_parser.getUnitCount())) {
        return QVariant();
    }

    const auto& unit = m_parser.getData().lyrics.at(row);

    if (role == Qt::DisplayRole) {
        return QString::fromStdString(unit.text);
    }
    if (role == UserDefineRole::CurrentTime) {  // user defined: TimeRole
        return static_cast<qlonglong>(unit.time_ms);
    }

    if (role == Qt::TextAlignmentRole) {
        return Qt::AlignCenter;
    }

    if (role == UserDefineRole::CurrentLine) {

    }
    return QVariant();
}
