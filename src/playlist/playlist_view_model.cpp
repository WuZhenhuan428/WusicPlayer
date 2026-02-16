#include "playlist_view_model.h"
#include <QFileInfo>
#include <QTime>
#include <QRegularExpression>
#include <QPointer>
#include <QThread>
#include <algorithm>
#include <random>
#include "../../include/audio.h"

PlaylistViewModel::PlaylistViewModel(PlaylistRepo* repo, QObject* parent)
    : QAbstractItemModel(parent)
    , m_repo(repo)
{
    initDefaultColumns();
    m_root = new Node();
    m_batchRebuildTimer = new QTimer(this);
    m_batchRebuildTimer->setSingleShot(true);
    m_batchRebuildTimer->setInterval(60);
    connect(m_batchRebuildTimer, &QTimer::timeout, this, &PlaylistViewModel::rebuildAsync);
    if (m_repo) {
        connect(m_repo, &PlaylistRepo::playlistChanged, this, &PlaylistViewModel::rebuildAsync);
        connect(m_repo, &PlaylistRepo::playlistBatchLoaded, this, [this](const QUuid& playlistId, int, int) {
            if (playlistId == m_playlistId) {
                scheduleBatchRebuild();
            }
        });
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

void PlaylistViewModel::scheduleBatchRebuild() {
    if (!m_batchRebuildTimer) {
        return;
    }
    if (!m_batchRebuildTimer->isActive()) {
        m_batchRebuildTimer->start();
    }
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

    if (!layout.updatedMeta.isEmpty()) {
        for (const auto& entry : layout.updatedMeta) {
            playlistPtr->updateTrackMeta(entry.id, entry.meta);
        }
        m_repo->saveListToCache(playlistPtr);
    }

    m_root = layout.root;
    m_playbackQueue = layout.playbackQueue;
    m_singleShuffleQueue = generateSingleShuffleQueue();
    m_groupShuffleQueue = generateGroupShuffleQueue();
    
    endResetModel();
    qDebug() << "[INFO] rebuild finished. Queue size:" << m_playbackQueue.size();
    emit changedPlaybackQueue();
}

void PlaylistViewModel::rebuildAsync() {
    const int token = ++m_rebuildToken;

    if (!m_repo) {
        beginResetModel();
        delete m_root;
        m_root = new Node();
        m_playbackQueue.clear();
        endResetModel();
        return;
    }

    auto playlistPtr = m_repo->findPlaylistById(m_playlistId);
    if (!playlistPtr) {
        beginResetModel();
        delete m_root;
        m_root = new Node();
        m_playbackQueue.clear();
        endResetModel();
        return;
    }

    const Playlist playlistCopy = *playlistPtr;
    PlaylistLayoutBuilder builderCopy = m_layoutBuilder;

    QPointer<PlaylistViewModel> self(this);
    QThread* worker = QThread::create([self, token, playlistCopy, builderCopy]() mutable {
        if (!self) {
            return;
        }

        LayoutResult layout = builderCopy.build(playlistCopy);
        if (!self) {
            delete layout.root;
            return;
        }

        QMetaObject::invokeMethod(self, [self, token, layout = std::move(layout)]() mutable {
            if (!self) {
                delete layout.root;
                return;
            }
            if (token != self->m_rebuildToken) {
                delete layout.root;
                return;
            }

            self->beginResetModel();
            delete self->m_root;
            self->m_root = layout.root;
            self->m_playbackQueue = layout.playbackQueue;
            self->m_singleShuffleQueue = self->generateSingleShuffleQueue();
            self->m_groupShuffleQueue = self->generateGroupShuffleQueue();
            self->endResetModel();

            auto playlistPtr = self->m_repo ? self->m_repo->findPlaylistById(self->m_playlistId) : nullptr;
            if (playlistPtr && !layout.updatedMeta.isEmpty()) {
                for (const auto& entry : layout.updatedMeta) {
                    playlistPtr->updateTrackMeta(entry.id, entry.meta);
                }
                self->m_repo->saveListToCache(playlistPtr);
            }
            emit self->changedPlaybackQueue();
        }, Qt::QueuedConnection);
    });

    QObject::connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}

void PlaylistViewModel::setPlaylist(const playlistId& playlist_id) {
    if (m_playlistId == playlist_id) { return; }
    m_playlistId = playlist_id;
    this->rebuildAsync();
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

    this->rebuildAsync();
}


void PlaylistViewModel::setSingleGrouping(SortRule rule) {
    m_layoutBuilder.updateSort(rule, false);
    this->rebuildAsync();
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
    if (role == Qt::TextAlignmentRole && index.column() == 0) {
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
    rebuildAsync();
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

QVector<trackId> PlaylistViewModel::generateGroupShuffleQueue() {
    if (!m_root || m_root->children.isEmpty()) {
        return {};
    }
    QVector<trackId> result;
    result.reserve(m_playbackQueue.size());

    // Just copy, do not use reference
    QVector<Node*> groups = m_root->children;

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(groups.begin(), groups.end(), g);
    for (Node* group : groups) {
        for (Node* track_node : group->children) {
            if (!track_node->id.isNull()) {
                result.append(track_node->id);
            }
        }
    }
    qDebug() << "[INFO] (re)build m_groupShuffleQueue";
    return result;
}

QVector<trackId> PlaylistViewModel::generateSingleShuffleQueue() {
    QVector<trackId> result;
    result.reserve(m_playbackQueue.size());
    result = m_playbackQueue;

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(m_singleShuffleQueue.begin(), m_singleShuffleQueue.end(), g);
    qDebug() << "[INFO] (re)build m_singleShuffleQueue";
    return result;
}

void PlaylistViewModel::setPlayMode(PlayMode to_mode) {
    if (m_playMode == to_mode) {
        return;
    } else {
        m_playMode = to_mode;
    }
}

const PlayMode PlaylistViewModel::getPlayMode() const {
    return this->m_playMode;
}

trackId PlaylistViewModel::nextOf(const trackId& track_id) const {
    int index;
    auto generate_random_index = [](size_t max_index = 0) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> dist(0, max_index);
        return dist(gen);
    };

    if (m_playMode == PlayMode::in_order) {
        index = m_playbackQueue.indexOf(track_id);
        if (index != -1 && index < m_playbackQueue.size() - 1) {
            return m_playbackQueue.at(index+1);
        } // else -> end of playlist
    } else if (m_playMode == PlayMode::loop) {
        index = m_playbackQueue.indexOf(track_id);
        if (index != -1)  {
            if (index < m_playbackQueue.size() - 1) {
                return m_playbackQueue.at(index+1);
            }
            else if (index = m_playbackQueue.size() - 1) {
                return m_playbackQueue.at(0);
            }
        }
    } else if (m_playMode == PlayMode::shuffle ) {
        index = generate_random_index(m_singleShuffleQueue.size()-1);
        return m_singleShuffleQueue.at(index);
    } else if (m_playMode == PlayMode::out_of_order_track) {
        index = m_singleShuffleQueue.indexOf(track_id);
        if (index != -1 && index <= m_singleShuffleQueue.size()-1) {
            return m_singleShuffleQueue.at(index+1);
        }
    } else if (m_playMode == PlayMode::out_of_order_group) {
        index = m_groupShuffleQueue.indexOf(track_id);
        if (index != -1 && index <= m_groupShuffleQueue.size()-1) {
            return m_groupShuffleQueue.at(index+1);
        }
    }
    return QUuid();
}

trackId PlaylistViewModel::previousOf(const trackId& track_id) const {
    int index;
    auto generate_random_index = [](size_t max_index = 0) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<size_t> dist(0, max_index);
        return dist(gen);
    };

    if (m_playMode == PlayMode::in_order) {
        index = m_playbackQueue.indexOf(track_id);
        if (index > 0) {    // -1 and 0
            return m_playbackQueue.at(index-1);
        } // else -> start of playlist
    } else if (m_playMode == PlayMode::loop) {
        index = m_playbackQueue.indexOf(track_id);
        if (index != -1)  {
            if (index > 0) {
                return m_playbackQueue.at(index-1);
            }
            else if (index == 0) {
                return m_playbackQueue.at(m_playbackQueue.size()-1);
            }
        }
    } else if (m_playMode == PlayMode::shuffle ) {
        index = generate_random_index(m_singleShuffleQueue.size()-1);
        return m_singleShuffleQueue.at(index);
    } else if (m_playMode == PlayMode::out_of_order_track) {
        index = m_singleShuffleQueue.indexOf(track_id);
        if (index > 0) {
            return m_singleShuffleQueue.at(index-1);
        }
    } else if (m_playMode == PlayMode::out_of_order_group) {
        index = m_groupShuffleQueue.indexOf(track_id);
        if (index > 0) {
            return m_groupShuffleQueue.at(index-1);
        }
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