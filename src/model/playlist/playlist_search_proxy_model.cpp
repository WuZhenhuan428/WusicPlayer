#include "playlist_search_proxy_model.h"

PlaylistSearchProxyModel::PlaylistSearchProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
    this->setDynamicSortFilter(true);
}

void PlaylistSearchProxyModel::setKeyword(const QString& keyword) {
    const QString trimmed = keyword.trimmed();
    if (m_keyword == trimmed) {
        return;
    }
    m_keyword = trimmed;
    
    if (m_keyword.isEmpty()) {
        m_regex = QRegularExpression();
    } else {
        m_regex = QRegularExpression(
            QRegularExpression::escape(m_keyword),
            QRegularExpression::CaseInsensitiveOption
        );
    }
    beginFilterChange();
    endFilterChange();
}

QString PlaylistSearchProxyModel::keyword() const {
    return m_keyword;
}

bool PlaylistSearchProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const {
    if (!sourceModel()) {
        return false;
    }
    
    if (m_keyword.isEmpty()) {
        return true;
    }

    if (rowMatches(sourceRow, sourceParent)) {
        return true;
    }

    if (hasAcceptedchildren(sourceRow, sourceParent)) {
        return true;
    }
    return false;
}

bool PlaylistSearchProxyModel::rowMatches(int source_row, const QModelIndex& source_parent) const {
    const QAbstractItemModel* model = this->sourceModel();
    if (!model) {
        return false;
    }

    const int col_cnt = model->columnCount(source_parent);
    for (int col = 0; col < col_cnt; ++col) {
        QModelIndex idx = model->index(source_row, col, source_parent);
        if (!idx.isValid()) {
            continue;
        }

        const QString text = model->data(idx, Qt::DisplayRole).toString();
        if (text.contains(m_regex)) {
            return true;
        }
    }
    return false;
}

bool PlaylistSearchProxyModel::hasAcceptedchildren(int source_row, const QModelIndex source_parent) const {
    const QAbstractItemModel* model = this->sourceModel();
    if (!model) {
        return false;
    }

    const QModelIndex current_index = model->index(source_row, 0, source_parent);
    if (!current_index.isValid()) {
        return false;
    }

    const int child_cnt = model->rowCount(current_index);
    for (int i = 0; i < child_cnt; ++i) {
        if (rowMatches(i, current_index) || hasAcceptedchildren(i, current_index)) {
            return true;
        }
    }

    return false;
}
