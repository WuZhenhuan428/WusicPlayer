#include "model/search_model/search_model.h"

#include "controller/search_backend/i_search_backend.h"
#include <utility>

namespace
{

QString format_duration(int total_seconds)
{
    if (total_seconds < 0) {
        return QStringLiteral("00:00");
    }

    const int minutes = total_seconds / 60;
    const int seconds = total_seconds % 60;

    return QStringLiteral("%1:%2")
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(seconds, 2, 10, QLatin1Char('0'));
}

} // namespace

SearchModel::SearchModel(ISearchBackend* backend, QObject* parent) :
    QAbstractTableModel(parent), backend_(backend)
{}

void SearchModel::search_request(SearchQuery query)
{
    last_query_ = std::move(query);

    if (last_query_.keyword.trimmed().isEmpty()) {
        clear_results();
        return;
    }

    if (!backend_) {
        clear_results();
        return;
    }

    const QVector<SearchHint> results = backend_->search(last_query_);
    set_results(results);
}

void SearchModel::set_results(const QVector<SearchHint>& results)
{
    beginResetModel();
    search_hint_ = results;
    endResetModel();
}

void SearchModel::set_backend(ISearchBackend* backend)
{
    backend_ = backend;
}

void SearchModel::clear_results()
{
    beginResetModel();
    search_hint_.clear();
    endResetModel();
}

TrackId SearchModel::track_id_at(int row) const
{
    if (row < 0 || row >= search_hint_.size()) {
        return TrackId{};
    }
    return search_hint_[row].track_id;
}

SearchHint SearchModel::hint_at(int row) const
{
    if (row < 0 || row >= search_hint_.size()) {
        return SearchHint{};
    }
    return search_hint_[row];
}

int SearchModel::total_hits() const
{
    return static_cast<int>(search_hint_.size());
}

const SearchQuery& SearchModel::last_query() const
{
    return last_query_;
}

QModelIndex SearchModel::index(int row, int column, const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return QModelIndex{};
    }
    if (!hasIndex(row, column, parent)) {
        return QModelIndex{};
    }
    return createIndex(row, column);
}

QModelIndex SearchModel::parent([[maybe_unused]] const QModelIndex& child) const
{
    return QModelIndex{};
}

int SearchModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(search_hint_.size());
}

int SearchModel::columnCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return static_cast<int>(Column::Count);
}

QVariant SearchModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant{};
    }
    if (index.row() < 0 || index.row() >= search_hint_.size()) {
        return QVariant{};
    }
    if (index.column() < 0 || index.column() >= static_cast<int>(Column::Count)) {
        return QVariant{};
    }

    const SearchHint& hint = search_hint_[index.row()];

    if (role == Qt::TextAlignmentRole) {
        if (index.column() == static_cast<int>(Column::Duration) ||
            index.column() == static_cast<int>(Column::Score)) {
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
        }
        return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
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
            return format_duration(hint.duration_s);
        case Column::Score:
            return static_cast<int>(hint.score);
        case Column::Count:
            break;
        }
    }

    return QVariant{};
}

QVariant SearchModel::headerData(int section, Qt::Orientation orientation, int role) const
{
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
