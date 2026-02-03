#include "playlist_view_model.h"
#include <QFileInfo>
#include <QTime>
#include <QRegularExpression>
#include <algorithm>
#include "../../include/audio.h"

PlaylistViewModel::PlaylistViewModel(PlaylistRepo* repo)
    : m_repo(repo)
{
    initDefaultColumns();
    m_root = new Node();
    if (m_repo) {
        connect(m_repo, &PlaylistRepo::playlistChanged, this, &PlaylistViewModel::rebuild);
    }
}

void PlaylistViewModel::initDefaultColumns() {
    m_columns = {
        {"", SortType::not_sorted},
        {"Disc", SortType::disc_number},
        {"#", SortType::track_number},
        {"Title", SortType::title},
        {"Artist", SortType::artist},
        {"Duration", SortType::duration},
        {"Album", SortType::album}
    };
}

PlaylistViewModel::~PlaylistViewModel() {
    delete m_root;
}

const Playlist& PlaylistViewModel::resolvePlaylist() {
    // Warning: This might crash if playlist is not found!
    // Prefer using findPlaylistById directly with check.
    return *(m_repo->findPlaylistById(m_playlistId));
}

void PlaylistViewModel::rebuild() {
    beginResetModel();

    // Clean old data
    delete m_root;

    if (!m_repo) {
        m_root = new Node();
        m_playbackQueue.clear();
        endResetModel();
        return;
    }
    
    // Fix: Check pointer validity to avoid Core Dump
    auto playlistPtr = m_repo->findPlaylistById(m_playlistId);
    if (!playlistPtr) {
        m_root = new Node();
        m_playbackQueue.clear();
        endResetModel();
        return;
    }

    const Playlist& pl = *playlistPtr;
        
    LayoutResult layout = m_layoutBuilder.build(pl);

    m_root = layout.root;
    m_playbackQueue = layout.playbackQueue;
    
    endResetModel();
    qDebug() << "[INFO] rebuild finished. Queue size:" << m_playbackQueue.size();
    emit changedPlaybackQueue();
}

void PlaylistViewModel::setPlaylist(const playlistId& playlist_id) {
    if (m_playlistId == playlist_id) { return; }
    m_playlistId = playlist_id;
    this->rebuild();
}

void PlaylistViewModel::setSortExpression(const QString& expression) {
    QStringList splited_exp = expression.split("|", Qt::SkipEmptyParts);
    QString group_expression;
    QString sort_expression;

    if (splited_exp.size() == 0) {      // str is null or contains `|` only
        return;
    } else if (splited_exp.size() == 1) {   // all group_exp
        group_expression = splited_exp[0];
    } else {
        group_expression = splited_exp[0];
        // if there are more than one `|`, only use the first `|`
        sort_expression = expression.mid(expression.indexOf("|") + 1);
    }

    auto extractKeys = [](const QString& str) -> QVector<QString> {
        QVector<QString> keys;
        if (str.trimmed().isEmpty()) {
            return keys;
        }

        QRegularExpression re(R"(%([^%]+)%)");
        QRegularExpressionMatchIterator it = re.globalMatch(str);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            keys.append(match.captured(1).trimmed());
        }
        return keys;
    };

    QVector<QString> group_list = extractKeys(group_expression);
    QVector<QString> sort_list = extractKeys(sort_expression);

    auto check_match = [](const QString& str) -> SortType {
        return mapStrToSorttype.value(str.toLower(), SortType::not_sorted);
    };

    auto string_to_sort_type = [check_match](const QVector<QString>& list) -> QVector<SortType> {
        QVector<SortType> types;
        for (const auto& it : list) {
            types.append(check_match(it));
        }
        return types;
    };

    QVector<SortType> group_type = string_to_sort_type(group_list);
    QVector<SortType> sort_type = string_to_sort_type(sort_list);

    QVector<SortRule> group_rule;
    QVector<SortRule> sort_rule;

    for (auto type : group_type) {
        SortRule rule = SortRule();
        rule.type = type;
        if (type==SortType::year) {
            rule.order = Qt::DescendingOrder;
        } // default: Qt::AscendingOrder
        group_rule.append(rule);
    }

    for (auto type : sort_type) {
        SortRule rule = SortRule();
        rule.type = type;
        if (type==SortType::year) {
            rule.order = Qt::DescendingOrder;
        } // default: Qt::AscendingOrder
        sort_rule.append(rule);
    }
    
    m_layoutBuilder.setGroupRule(group_rule);
    m_layoutBuilder.setSortRule(sort_rule);

    this->rebuild();
}


void PlaylistViewModel::setSingleGrouping(SortRule rule) {
    m_layoutBuilder.updateSort(rule, false);
    this->rebuild();
}


void PlaylistViewModel::setActiveTrack(const trackId& track_id) {
    // Helper lambda to find index
    auto findIndex = [this](const trackId& id) -> QModelIndex {
        if (id.isNull()) return QModelIndex();
        for(int i=0; i<m_root->children.size(); ++i) {
            Node* group = m_root->children[i];
            for(int j=0; j<group->children.size(); ++j) {
                if (group->children[j]->id == id) {
                    return createIndex(j, 0, group->children[j]);
                }
            }
        }
        return QModelIndex();
    };

    // 1. Refresh old track row
    if (!m_activeTrackId.isNull()) {
        QModelIndex oldIdx = findIndex(m_activeTrackId);
        if (oldIdx.isValid()) {
            emit dataChanged(oldIdx, oldIdx, {Qt::DisplayRole});
        }
    }

    m_activeTrackId = track_id;

    // 2. Refresh new track row
    if (!m_activeTrackId.isNull()) {
        QModelIndex newIdx = findIndex(m_activeTrackId);
        if (newIdx.isValid()) {
            emit dataChanged(newIdx, newIdx, {Qt::DisplayRole});
        }
    }
}

void PlaylistViewModel::clear() {
    beginResetModel();
    delete m_root;
    m_root = new Node();
    m_playbackQueue.clear();
    endResetModel();
}

/* ==== QAbstractItemModel Interface ==== */

QModelIndex PlaylistViewModel::index(int row, int column, const QModelIndex &parent) const {
    if (!hasIndex(row, column, parent)) return QModelIndex();

    Node* parentNode;
    if (!parent.isValid())
        parentNode = m_root;
    else
        parentNode = static_cast<Node*>(parent.internalPointer());

    if (row < 0 || row >= parentNode->children.size()) return QModelIndex();
    
    Node* childNode = parentNode->children.at(row);
    return createIndex(row, column, childNode);
}

QModelIndex PlaylistViewModel::parent(const QModelIndex &child) const {
    if (!child.isValid()) return QModelIndex();

    Node* childNode = static_cast<Node*>(child.internalPointer());
    Node* parentNode = childNode->parent;

    if (parentNode == m_root || !parentNode) return QModelIndex();

    return createIndex(parentNode->row(), 0, parentNode);
}

int PlaylistViewModel::rowCount(const QModelIndex &parent) const {
    Node* parentNode;
    if (parent.column() > 0) return 0;

    if (!parent.isValid())
        parentNode = m_root;
    else
        parentNode = static_cast<Node*>(parent.internalPointer());

    return parentNode->children.size();
}

int PlaylistViewModel::columnCount(const QModelIndex &parent) const {
    return m_columns.size();
}

QVariant PlaylistViewModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) return QVariant();

    Node* node = static_cast<Node*>(index.internalPointer());
    bool isGroup = node->id.isNull(); 

    if (role == Qt::TextAlignmentRole && isGroup) {
        return int(Qt::AlignLeft | Qt::AlignVCenter);
    }

    if (role == Qt::DisplayRole) {
        // Group Logic
        if (isGroup) {
            if (index.column() == 0) {
                return node->groupName + QString(" (%1)").arg(node->children.size());
            }
            return QVariant();
        }

        // Track Logic
        const TrackMetaData& d = node->meta;
        if (index.column() < 0 || index.column() >= m_columns.size()) return QVariant();
        
        const TableColumn& col = m_columns[index.column()];

        if (col.sortType == SortType::not_sorted) {
             return (node->id == m_activeTrackId) ? ">" : "";
        }
        
        // Special formatting
        if (col.sortType == SortType::duration) {
             int time_s = d.duration_s;
             int hours = time_s / 3600;
             int mins = (time_s % 3600) / 60;
             int secs = time_s % 60;
             if (hours > 0) {
                 return QString("%1:%2:%3")
                     .arg(hours, 2, 10, QChar('0'))
                     .arg(mins, 2, 10, QChar('0'))
                     .arg(secs, 2, 10, QChar('0'));
             }
             return QString("%1:%2").arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
        }

        // Default meta data retrieval
        QVariant val = PlaylistLayoutBuilder::getMetaDataValue(d, col.sortType);
        if (val.isValid()) return val;
        return QVariant();
    }
    return QVariant();
}

QVariant PlaylistViewModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole) return QVariant();
    if (orientation == Qt::Horizontal) {
        if (section >= 0 && section < m_columns.size()) {
            return m_columns[section].headerName;
        }
    }
    return QVariant();
}

void PlaylistViewModel::sort(int column, Qt::SortOrder order) {
    if (column < 0 || column >= m_columns.size()) return;
    
    SortType type = m_columns[column].sortType;
    if (type == SortType::not_sorted) return;

    SortRule rule;
    rule.type = type;
    rule.order = order;
    
    m_layoutBuilder.setSortRule({rule});
    rebuild();
}

/* ==== Helpers ==== */

trackId PlaylistViewModel::trackAt(int index) const {
    if (index >= 0 && index < m_playbackQueue.size())
        return m_playbackQueue.at(index);
    return QUuid();
}

trackId PlaylistViewModel::trackAt(const QModelIndex& index) const {
    if (!index.isValid()) return QUuid();
    Node* node = static_cast<Node*>(index.internalPointer());
    return node->id; 
}

const QVector<trackId>& PlaylistViewModel::playbackQueue() const {
    return m_playbackQueue;
}


trackId PlaylistViewModel::nextOf(const trackId& track_id) const {
    int idx = m_playbackQueue.indexOf(track_id);
    if (idx != -1 && idx < m_playbackQueue.size() - 1) {
        return m_playbackQueue.at(idx + 1);
    }
    return QUuid();
}

trackId PlaylistViewModel::previousOf(const trackId& track_id) const {
    int idx = m_playbackQueue.indexOf(track_id);
    if (idx > 0) {
        return m_playbackQueue.at(idx - 1);
    }
    return QUuid();
}

/* ==== Dynamic Column Management ==== */

void PlaylistViewModel::insertColumn(int index, const TableColumn& column) {
    if (index < 0 || index > m_columns.size()) return;
    beginInsertColumns(QModelIndex(), index, index);
    m_columns.insert(index, column); 
    endInsertColumns();
}

void PlaylistViewModel::removeColumn(int index) {
    if (index < 0 || index >= m_columns.size()) return;
    beginRemoveColumns(QModelIndex(), index, index);
    m_columns.removeAt(index);
    endRemoveColumns();
}

void PlaylistViewModel::setColumns(const QVector<TableColumn>& columns) {
    beginResetModel();
    m_columns = columns;
    endResetModel();
}

const QVector<TableColumn>& PlaylistViewModel::getColumns() const {
    return m_columns;
}

void PlaylistViewModel::requestMetaData(const trackId& track_id) {
    // Sync parsing used currently
}