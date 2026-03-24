#include "search_model.h"

#include "controller/search_backend/i_search_backend.h"

#include <utility>

namespace {

QString formatDuration(int total_seconds) {
    if (total_seconds < 0) {
        return QStringLiteral("00:00");
    }

    const int minutes = total_seconds / 60;
    const int seconds = total_seconds % 60;

    return QStringLiteral("%1:%2")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));
}

}

SearchModel::SearchModel(ISearchBackend* backend, QObject* parent)
    : QAbstractTableModel(parent), m_backend(backend)
{
}

SearchModel::~SearchModel() {}

void SearchModel::searchRequest(SearchQuery query) {
    m_last_query = std::move(query);

    if (m_last_query.keyword.trimmed().isEmpty()) {
        clearResults();
        return;
    }

    if (!m_backend) {
        clearResults();
        return;
    }

    const QVector<SearchHint> results = m_backend->search(m_last_query);
    setResults(results);
}

void SearchModel::setResults(const QVector<SearchHint>& results) {
    beginResetModel();
    m_search_hint = results;
    endResetModel();
}

void SearchModel::setBackend(ISearchBackend* backend) {
    m_backend = backend;
}

void SearchModel::clearResults() {
    beginResetModel();
    m_search_hint.clear();
    endResetModel();
}

trackId SearchModel::trackIdAt(int row) const {
    if (row < 0 || row >= m_search_hint.size()) {
        return trackId{};
    }
    return m_search_hint[row].track_id;
}

SearchHint SearchModel::hintAt(int row) const {
    if (row < 0 || row >= m_search_hint.size()) {
        return SearchHint{};
    }
    return m_search_hint[row];
}

int SearchModel::totalHits() const {
    return m_search_hint.size();
}

const SearchQuery& SearchModel::lastQuery() const {
    return m_last_query;
}

QModelIndex SearchModel::index(int row, int column, const QModelIndex &parent) const {
    if (parent.isValid()) {
        return QModelIndex{};
    }
    if (!hasIndex(row, column, parent)) {
        return QModelIndex{};
    }
    return createIndex(row, column);
}

QModelIndex SearchModel::parent(const QModelIndex &child) const {
    Q_UNUSED(child);
    return QModelIndex{};
}

int SearchModel::rowCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return m_search_hint.size();
}

int SearchModel::columnCount(const QModelIndex &parent) const {
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(Column::Count);
}

QVariant SearchModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) {
        return QVariant{};
    }
    if (index.row() < 0 || index.row() >= m_search_hint.size()) {
        return QVariant{};
    }
    if (index.column() < 0 || index.column() >= static_cast<int>(Column::Count)) {
        return QVariant{};
    }

    const SearchHint& hint = m_search_hint[index.row()];

    if (role == Qt::TextAlignmentRole) {
        if (index.column() == static_cast<int>(Column::Duration)
            || index.column() == static_cast<int>(Column::Score)) {
            return int(Qt::AlignRight | Qt::AlignVCenter);
        }
        return int(Qt::AlignLeft | Qt::AlignVCenter);
    }

    if (role == Qt::DisplayRole) {
        switch (static_cast<Column>(index.column())) {
        case Column::Title:
            return hint.title;
        case Column::Artist:
            return hint.artist;
        case Column::AlbumArtist:
            return hint.album_artist;
        case Column::Album:
            return hint.album;
        case Column::Duration:
            return formatDuration(hint.duration_s);
        case Column::Score:
            return int(hint.score);
        case Column::Count:
            break;
        }
    }

    return QVariant{};
}

QVariant SearchModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole) {
        return QVariant{};
    }

    if (orientation == Qt::Horizontal) {
        switch (static_cast<Column>(section)) {
        case Column::Title:
            return QStringLiteral("Title");
        case Column::Artist:
            return QStringLiteral("Artist");
        case Column::AlbumArtist:
            return QStringLiteral("Album Artist");
        case Column::Album:
            return QStringLiteral("Album");
        case Column::Duration:
            return QStringLiteral("Duration");
        case Column::Score:
            return QStringLiteral("Score");
        case Column::Count:
            return QVariant{};
        }
    }

    return QVariant{};
}
