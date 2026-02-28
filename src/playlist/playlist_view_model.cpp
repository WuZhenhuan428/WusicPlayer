#include "playlist_view_model.h"
#include <QFileInfo>
#include <QTime>
#include <QRegularExpression>
#include <QPointer>
#include <QThread>
// #include <algorithm>
#include <random>
#include "../../src/core/utils/AudioUtils.h"

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
        connect(m_repo, &PlaylistRepo::playlistBatchLoaded, this, [this](const playlistId& playlist_id, int, int) {
            if (playlist_id == m_pid) {
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
    return *(m_repo->findPlaylistById(m_pid));
}

void PlaylistViewModel::rebuild() {
    beginResetModel();

    // Clean old data
    delete m_root;

    if (!m_repo) {
        m_root = new Node();
        m_playbackQueue.clear();
        endResetModel();
        m_activeTrackIndex = QPersistentModelIndex(findTrackIndex(m_activeTrackId));
        return;
    }
    
    // Fix: Check pointer validity to avoid Core Dump
    auto playlist_ptr = m_repo->findPlaylistById(m_pid);
    if (!playlist_ptr) {
        m_root = new Node();
        m_playbackQueue.clear();
        endResetModel();
        m_activeTrackIndex = QPersistentModelIndex(findTrackIndex(m_activeTrackId));
        return;
    }

    const Playlist& pl = *playlist_ptr;

    LayoutResult layout = m_layoutBuilder.build(pl);

    if (!layout.updatedMeta.isEmpty()) {
        for (const auto& entry : layout.updatedMeta) {
            playlist_ptr->updateTrackMeta(entry.id, entry.meta);
        }
        m_repo->saveListToCache(playlist_ptr);
    }

    m_root = layout.root;
    m_playbackQueue = layout.playbackQueue;
    m_singleShuffleQueue = generateSingleShuffleQueue();
    m_groupShuffleQueue = generateGroupShuffleQueue();
    
    endResetModel();
    m_activeTrackIndex = QPersistentModelIndex(findTrackIndex(m_activeTrackId));
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
        m_activeTrackIndex = QPersistentModelIndex(findTrackIndex(m_activeTrackId));
        return;
    }

    auto playlistPtr = m_repo->findPlaylistById(m_pid);
    if (!playlistPtr) {
        beginResetModel();
        delete m_root;
        m_root = new Node();
        m_playbackQueue.clear();
        endResetModel();
        m_activeTrackIndex = QPersistentModelIndex(findTrackIndex(m_activeTrackId));
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
            self->m_activeTrackIndex = QPersistentModelIndex(self->findTrackIndex(self->m_activeTrackId));

            auto playlistPtr = self->m_repo ? self->m_repo->findPlaylistById(self->m_pid) : nullptr;
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

void PlaylistViewModel::setPlaylist(const playlistId& pid) {
    if (m_pid == pid) { return; }
    m_pid = pid;
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


void PlaylistViewModel::setActiveTrack(const trackId& tid) {
    QModelIndex old_index = getCurrentTrackIndex();
    m_activeTrackId = tid;
    m_activeTrackIndex = QPersistentModelIndex(findTrackIndex(tid));

    QModelIndex new_index = getCurrentTrackIndex();

    auto emitRowChanged = [this](const QModelIndex& index) {
        if (!index.isValid()) return;
        const QModelIndex left = index.siblingAtColumn(0);
        const QModelIndex right = index.siblingAtColumn(std::max(0, columnCount()-1));
        emit dataChanged(left, right, {Qt::DisplayRole});
    };

    emitRowChanged(old_index);
    emitRowChanged(new_index);
}

void PlaylistViewModel::clear() {
    beginResetModel();
    delete m_root;
    m_root = new Node();
    m_playbackQueue.clear();
    endResetModel();
    m_activeTrackIndex = QPersistentModelIndex();
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

PlaybackQueueSnapshot PlaylistViewModel::playbackQueueSnapshot() const {
    static int version = 0;
    return {m_playbackQueue, version++};
}

PlaybackQueueSnapshot PlaylistViewModel::singleShuffleQueueSnapshot() const {
    static int version = 0;
    return {m_singleShuffleQueue, version++};
}

PlaybackQueueSnapshot PlaylistViewModel::groupShuffleQueueSnapshot() const {
    static int version = 0;
    return {m_groupShuffleQueue, version++};
}


trackId PlaylistViewModel::trackAt(int index) const {
    if (index >= 0 && index < m_playbackQueue.size())
        return m_playbackQueue.at(index);
    return trackId();
}

trackId PlaylistViewModel::trackAt(const QModelIndex& index) const {
    if (!index.isValid()) return trackId();
    Node* node = static_cast<Node*>(index.internalPointer());
    return node->id; 
}

QModelIndex PlaylistViewModel::getCurrentTrackIndex() {
    if (m_activeTrackIndex.isValid()) {
        return m_activeTrackIndex;
    }
    return findTrackIndex(m_activeTrackId);
}

QModelIndex PlaylistViewModel::findTrackIndex(const trackId& tid) const {
    if (tid.isNull() || !m_root) {
        return QModelIndex();
    }

    for (int group_row = 0; group_row < m_root->children.size(); ++group_row) {
        Node* group = m_root->children.at(group_row);
        for (int track_row = 0; track_row < group->children.size(); ++track_row) {
            Node* track_node = group->children.at(track_row);
            if (track_node->id == tid) {
                QModelIndex parent_index = createIndex(group_row, 0, group);
                return createIndex(track_row, 0, track_node);
            }
        }
    }
    return QModelIndex();
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
    if (!m_root || m_root->children.isEmpty()) {
        return {};
    }
    QVector<trackId> result;
    result.reserve(m_playbackQueue.size());
    result = m_playbackQueue;

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(result.begin(), result.end(), g);
    m_singleShuffleQueue.clear();
    m_singleShuffleQueue = result;
    qDebug() << "[INFO] (re)build m_singleShuffleQueue";
    return result;
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

void PlaylistViewModel::requestMetaData(const trackId& tid) {
    // Sync parsing used currently
}