#include "WLyricsModel.h"

#include <QFileInfo>
#include <QDir>
#include <algorithm>

WLyricsModel::WLyricsModel(QObject* parent)
    : QAbstractListModel(parent)
{

}

WLyricsModel::~WLyricsModel() {}

void WLyricsModel::setDefaultInfo() {
    beginResetModel();
    m_parser.clear();
    QString foo("[00:00.00] Wusic Player");
    m_parser.parseString(foo.toUtf8().toStdString());
    m_current_row = -1;
    endResetModel();

    emit currentLineChanged(QStringLiteral("Wusic Player"), QString());
}

bool WLyricsModel::setRawLyrics(const QString& raw_data) {
    beginResetModel();
    m_parser.clear();
    m_current_row = -1;
    if (raw_data.isEmpty()) {
        endResetModel();
        emit currentLineChanged(QString(), QString());
        return false;
    }
    if (!m_parser.parseString(raw_data.toStdString())) {
        endResetModel();
        emit currentLineChanged(QString(), QString());
        return false;
    }
    endResetModel();
    emit currentLineChanged(QString(), QString());
    return true;
}

bool WLyricsModel::setLocalLrc(const QString& filepath) {
    beginResetModel();
    m_parser.clear();
    m_current_row = -1;
    if (filepath.isEmpty()) {
        endResetModel();
        emit currentLineChanged(QString(), QString());
        return false;
    }
    QFileInfo audio_fileinfo(filepath);
    if (!audio_fileinfo.exists()) {
        qDebug() << "[WARNING] Audio file does not exist: " << filepath;
        endResetModel();
        emit currentLineChanged(QString(), QString());
        return false;
    }
    QString lrc_path = audio_fileinfo.path() + "/" + audio_fileinfo.completeBaseName() + ".lrc";
    QFileInfo possibel_lrc_fileinfo(lrc_path);
    if (possibel_lrc_fileinfo.exists() && possibel_lrc_fileinfo.isFile()) {
        m_parser.parseFile(lrc_path.toStdString());
        endResetModel();
        emit currentLineChanged(QString(), QString());
        return true;
    }
    endResetModel();
    emit currentLineChanged(QString(), QString());
    return false;
}

int WLyricsModel::getRowByPosition(qint64 pos_ms) {
    const auto& lyrics = m_parser.getData().lyrics;
    if (lyrics.empty()) {
        return -1;
    }
    // dichotomy
    auto it = std::upper_bound(lyrics.begin(), lyrics.end(), pos_ms,
        [](qint64 ms, const LrcUnit& unit){
            return ms < static_cast<qint64>(unit.time_ms);
        }
    );
    if (it == lyrics.begin()) {
        return 0;
    }
    return std::distance(lyrics.begin(), it) - 1;
}

int WLyricsModel::currentRow() const {
    return m_current_row;
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
        return row == m_current_row;
    }
    return QVariant();
}


void WLyricsModel::setCurrentPosition(qint64 pos_ms) {
    const int new_row = getRowByPosition(pos_ms);
    if (new_row == m_current_row) return;

    const int old_row = m_current_row;
    m_current_row = new_row;

    if (old_row >= 0 && old_row < rowCount()) {
        const QModelIndex idx = index(old_row, 0);
        emit dataChanged(idx, idx, {UserDefineRole::CurrentLine});
    }
    if (m_current_row >= 0 && m_current_row < rowCount()) {
        const QModelIndex idx = index(m_current_row, 0);
        emit dataChanged(idx, idx, {UserDefineRole::CurrentLine});
        emit currentLineChanged(currentLineText(), nextLineText());
    } else {
        emit currentLineChanged(QString(), QString());
    }
}

QString WLyricsModel::prevLineText() const {
    if (m_current_row < 1 || m_current_row >= rowCount() + 1) return QString();
    return data(index(m_current_row-1, 0), Qt::DisplayRole).toString();
}

QString WLyricsModel::currentLineText() const {
    if (m_current_row < 0 || m_current_row >= rowCount()) return QString();
    return data(index(m_current_row, 0), Qt::DisplayRole).toString();
}

QString WLyricsModel::nextLineText() const {
    if (m_current_row < -1 || m_current_row >= rowCount() - 1)  return QString();
    return data(index(m_current_row+1, 0), Qt::DisplayRole).toString();
}