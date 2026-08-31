#include "model/playlist/playlist_view_model.h"

#include <QColor>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeData>
#include <QPointer>
#include <QRegularExpression>
#include <QThread>
#include <QTime>
#include <random>

#include "core/logger/logger_manager.h"
namespace
{
Logger* logger = LoggerManager::file_logger("playlist_view_model", {"console", "gui"});
}

PlaylistViewModel::PlaylistViewModel(PlaylistRepo* repo, QObject* parent) :
    QAbstractItemModel(parent), m_repo(repo)
{
    init_default_columns();
    m_root                = new Node();
    m_batch_rebuild_timer = new QTimer(this);
    m_batch_rebuild_timer->setSingleShot(true);
    m_batch_rebuild_timer->setInterval(60);
    connect(m_batch_rebuild_timer, &QTimer::timeout, this, &PlaylistViewModel::rebuild_async);
    if (m_repo) {
        connect(m_repo, &PlaylistRepo::sgn_playlist_changed, this,
                &PlaylistViewModel::rebuild_async);
        connect(m_repo, &PlaylistRepo::playlistBatchLoaded, this,
                [this](const PlaylistId& playlist_id, int, int) {
                    if (playlist_id == m_pid) {
                        schedule_batch_rebuild();
                    }
                });
    }
}

void PlaylistViewModel::init_default_columns()
{
    m_columns = {{"", SortType::not_sorted},    {"Disc", SortType::disc_number},
                 {"#", SortType::track_number}, {"Title", SortType::title},
                 {"Artist", SortType::artist},  {"Duration", SortType::duration},
                 {"Album", SortType::album}};
}

PlaylistViewModel::~PlaylistViewModel()
{
    delete m_root;
}

void PlaylistViewModel::schedule_batch_rebuild()
{
    if (!m_batch_rebuild_timer) {
        return;
    }
    if (!m_batch_rebuild_timer->isActive()) {
        m_batch_rebuild_timer->start();
    }
}

const Playlist& PlaylistViewModel::resolve_playlist()
{
    // Warning: This might crash if playlist is not found!
    // Prefer using find_playlist_by_id directly with check.
    return *(m_repo->find_playlist_by_id(m_pid));
}

void PlaylistViewModel::rebuild()
{
    beginResetModel();

    // Clean old data
    delete m_root;

    if (!m_repo) {
        m_root = new Node();
        m_playback_queue.clear();
        endResetModel();
        m_active_track_index = QPersistentModelIndex(find_track_index(m_active_track_id));
        return;
    }

    // Fix: Check pointer validity to avoid Core Dump
    auto playlist_ptr = m_repo->find_playlist_by_id(m_pid);
    if (!playlist_ptr) {
        m_root = new Node();
        m_playback_queue.clear();
        endResetModel();
        m_active_track_index = QPersistentModelIndex(find_track_index(m_active_track_id));
        return;
    }

    const Playlist& pl  = *playlist_ptr;

    LayoutResult layout = m_layout_builder.build(pl);

    if (!layout.updated_meta.isEmpty()) {
        for (const auto& entry : layout.updated_meta) {
            playlist_ptr->update_track_meta(entry.id, entry.meta);
        }
        m_repo->save_list_to_cache(playlist_ptr);
    }

    m_root                 = layout.root;
    m_playback_queue       = layout.playback_queue;
    m_single_shuffle_queue = generate_single_shuffle_queue();
    m_group_shuffle_queue  = generate_group_shuffle_queue();

    endResetModel();
    m_active_track_index = QPersistentModelIndex(find_track_index(m_active_track_id));
    logger->info("rebuild finished. Queue size: {}", m_playback_queue.size());
    emit changedPlaybackQueue();
}

void PlaylistViewModel::rebuild_async()
{
    const int token = ++m_rebuild_token;

    if (!m_repo) {
        beginResetModel();
        delete m_root;
        m_root = new Node();
        m_playback_queue.clear();
        endResetModel();
        m_active_track_index = QPersistentModelIndex(find_track_index(m_active_track_id));
        return;
    }

    auto playlistPtr = m_repo->find_playlist_by_id(m_pid);
    if (!playlistPtr) {
        beginResetModel();
        delete m_root;
        m_root = new Node();
        m_playback_queue.clear();
        endResetModel();
        m_active_track_index = QPersistentModelIndex(find_track_index(m_active_track_id));
        return;
    }

    auto playlistSnapshot             = std::make_shared<Playlist>(*playlistPtr);
    PlaylistLayoutBuilder builderCopy = m_layout_builder;

    QPointer<PlaylistViewModel> self(this);
    QThread* worker = QThread::create([self, token, playlistSnapshot, builderCopy]() mutable {
        if (!self) {
            return;
        }

        LayoutResult layout = builderCopy.build(*playlistSnapshot);
        if (!self) {
            delete layout.root;
            return;
        }

        QMetaObject::invokeMethod(
            self,
            [self, token, layout = std::move(layout)]() mutable {
                if (!self) {
                    delete layout.root;
                    return;
                }
                if (token != self->m_rebuild_token) {
                    delete layout.root;
                    return;
                }

                self->beginResetModel();
                delete self->m_root;
                self->m_root                 = layout.root;
                self->m_playback_queue       = layout.playback_queue;
                self->m_single_shuffle_queue = self->generate_single_shuffle_queue();
                self->m_group_shuffle_queue  = self->generate_group_shuffle_queue();
                self->endResetModel();
                self->m_active_track_index =
                    QPersistentModelIndex(self->find_track_index(self->m_active_track_id));

                auto playlistPtr =
                    self->m_repo ? self->m_repo->find_playlist_by_id(self->m_pid) : nullptr;
                if (playlistPtr && !layout.updated_meta.isEmpty()) {
                    for (const auto& entry : layout.updated_meta) {
                        playlistPtr->update_track_meta(entry.id, entry.meta);
                    }
                    self->m_repo->save_list_to_cache(playlistPtr);
                }
                emit self->changedPlaybackQueue();
            },
            Qt::QueuedConnection);
    });

    QObject::connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}

void PlaylistViewModel::set_playlist(const PlaylistId& pid)
{
    if (m_pid == pid) {
        return;
    }
    m_pid = pid;
    this->rebuild_async();
}

void PlaylistViewModel::set_sort_expression(const QString& expression)
{
    // DSL 路径(排序/分组/分类); 空串 → 清除 DSL, 回退内置规则
    m_layout_builder.set_dsl(expression);
    this->rebuild_async();
}

void PlaylistViewModel::set_single_grouping(SortRule rule)
{
    m_layout_builder.update_sort(rule, false);
    this->rebuild_async();
}

void PlaylistViewModel::set_active_track(const EntryId& tid)
{
    QModelIndex old_index = get_current_track_index();
    m_active_track_id     = tid;
    m_active_track_index  = QPersistentModelIndex(find_track_index(tid));

    QModelIndex new_index = get_current_track_index();

    auto emitRowChanged   = [this](const QModelIndex& index) {
        if (!index.isValid())
            return;
        const QModelIndex left  = index.siblingAtColumn(0);
        const QModelIndex right = index.siblingAtColumn(std::max(0, columnCount() - 1));
        emit dataChanged(left, right, {Qt::DisplayRole});
    };

    emitRowChanged(old_index);
    emitRowChanged(new_index);
}

void PlaylistViewModel::clear()
{
    beginResetModel();
    delete m_root;
    m_root = new Node();
    m_playback_queue.clear();
    endResetModel();
    m_active_track_index = QPersistentModelIndex();
}

/* ==== QAbstractItemModel Interface ==== */

QModelIndex PlaylistViewModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    Node* parentNode;
    if (!parent.isValid())
        parentNode = m_root;
    else
        parentNode = static_cast<Node*>(parent.internalPointer());

    if (row < 0 || row >= parentNode->children.size())
        return QModelIndex();

    Node* childNode = parentNode->children.at(row);
    return createIndex(row, column, childNode);
}

QModelIndex PlaylistViewModel::parent(const QModelIndex& child) const
{
    if (!child.isValid())
        return QModelIndex();

    Node* childNode  = static_cast<Node*>(child.internalPointer());
    Node* parentNode = childNode->parent;

    if (parentNode == m_root || !parentNode)
        return QModelIndex();

    return createIndex(parentNode->row(), 0, parentNode);
}

int PlaylistViewModel::rowCount(const QModelIndex& parent) const
{
    Node* parentNode;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentNode = m_root;
    else
        parentNode = static_cast<Node*>(parent.internalPointer());

    return parentNode->children.size();
}

int PlaylistViewModel::columnCount([[maybe_unused]] const QModelIndex& parent) const
{
    return m_columns.size();
}

QVariant PlaylistViewModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    Node* node    = static_cast<Node*>(index.internalPointer());
    bool is_group = node->id.is_null();

    if (role == Qt::TextAlignmentRole && is_group) {
        return int(Qt::AlignLeft | Qt::AlignVCenter);
    }
    if (role == Qt::TextAlignmentRole && index.column() == 0) {
        return int(Qt::AlignLeft | Qt::AlignVCenter);
    }

    // 缺失文件置灰
    if (role == Qt::ForegroundRole && !is_group && node->missing) {
        return QColor(128, 128, 128);
    }

    if (role == Qt::DisplayRole) {
        // Group Logic
        if (is_group) {
            if (index.column() == 0) {
                return node->group_name + QString(" (%1)").arg(node->children.size());
            }
            return QVariant();
        }

        // Track Logic
        const TrackMetaData& d = node->meta;
        if (index.column() < 0 || index.column() >= m_columns.size())
            return QVariant();

        const TableColumn& col = m_columns[index.column()];

        if (col.sortType == SortType::not_sorted) {
            return (node->id == m_active_track_id) ? ">" : "";
        }

        // Special formatting
        if (col.sortType == SortType::duration) {
            int time_s = d.duration_s;
            int hours  = time_s / 3600;
            int mins   = (time_s % 3600) / 60;
            int secs   = time_s % 60;
            if (hours > 0) {
                return QString("%1:%2:%3")
                    .arg(hours, 2, 10, QChar('0'))
                    .arg(mins, 2, 10, QChar('0'))
                    .arg(secs, 2, 10, QChar('0'));
            }
            return QString("%1:%2").arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0'));
        }

        // Default meta data retrieval
        QVariant val = PlaylistLayoutBuilder::get_metadata_value(d, col.sortType);
        if (val.isValid())
            return val;
        return QVariant();
    }
    return QVariant();
}

QVariant PlaylistViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();
    if (orientation == Qt::Horizontal) {
        if (section >= 0 && section < m_columns.size()) {
            return m_columns[section].headerName;
        }
    }
    return QVariant();
}

void PlaylistViewModel::sort(int column, Qt::SortOrder order)
{
    if (column < 0 || column >= m_columns.size())
        return;

    SortType type = m_columns[column].sortType;
    if (type == SortType::not_sorted)
        return;

    SortRule rule;
    rule.type  = type;
    rule.order = order;

    m_layout_builder.set_sort_rule({rule});
    rebuild_async();
}

/* ==== Helpers ==== */

PlaybackQueueSnapshot PlaylistViewModel::playback_queue_snapshot() const
{
    static int version = 0;
    return {m_playback_queue, version++};
}

PlaybackQueueSnapshot PlaylistViewModel::single_shuffle_queue_snapshot() const
{
    static int version = 0;
    return {m_single_shuffle_queue, version++};
}

PlaybackQueueSnapshot PlaylistViewModel::group_shuffle_queue_snapshot() const
{
    static int version = 0;
    return {m_group_shuffle_queue, version++};
}

EntryId PlaylistViewModel::track_at(int index) const
{
    if (index >= 0 && index < m_playback_queue.size())
        return m_playback_queue.at(index);
    return EntryId();
}

EntryId PlaylistViewModel::track_at(const QModelIndex& index) const
{
    if (!index.isValid())
        return EntryId();
    Node* node = static_cast<Node*>(index.internalPointer());
    return node->id;
}

Qt::ItemFlags PlaylistViewModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags f = QAbstractItemModel::flags(index);
    if (index.isValid()) {
        f |= Qt::ItemIsDragEnabled;
    }
    return f;
}

QStringList PlaylistViewModel::mimeTypes() const
{
    return {QString::fromLatin1(wusic::kPlaylistEntriesMime)};
}

QMimeData* PlaylistViewModel::mimeData(const QModelIndexList& indexes) const
{
    // 收集选中曲目(过滤组节点:id 为空)
    QVector<EntryId> ids;
    for (const QModelIndex& index : indexes) {
        if (!index.isValid()) {
            continue;
        }
        auto* node = static_cast<Node*>(index.internalPointer());
        if (node && !node->id.is_null() && !ids.contains(node->id)) {
            ids.push_back(node->id);
        }
    }
    if (ids.isEmpty()) {
        return nullptr;
    }
    QJsonArray arr;
    for (const EntryId& id : ids) {
        arr.append(id.to_string_without_brace());
    }
    QJsonObject root;
    root["src"] = m_pid.to_string_without_brace();
    root["ids"] = arr;
    auto* mime  = new QMimeData;
    mime->setData(QString::fromLatin1(wusic::kPlaylistEntriesMime),
                  QJsonDocument(root).toJson(QJsonDocument::Compact));
    return mime;
}

QModelIndex PlaylistViewModel::get_current_track_index()
{
    if (m_active_track_index.isValid()) {
        return m_active_track_index;
    }
    return find_track_index(m_active_track_id);
}

QModelIndex PlaylistViewModel::find_track_index(const EntryId& tid) const
{
    if (tid.is_null() || !m_root) {
        return QModelIndex();
    }

    for (int group_row = 0; group_row < m_root->children.size(); ++group_row) {
        Node* group = m_root->children.at(group_row);
        for (int track_row = 0; track_row < group->children.size(); ++track_row) {
            Node* track_node = group->children.at(track_row);
            if (track_node->id == tid) {
                // QModelIndex parent_index = createIndex(group_row, 0, group); // unused
                return createIndex(track_row, 0, track_node);
            }
        }
    }
    return QModelIndex();
}

const QVector<EntryId>& PlaylistViewModel::playback_queue() const
{
    return m_playback_queue;
}

QVector<EntryId> PlaylistViewModel::generate_group_shuffle_queue()
{
    if (!m_root || m_root->children.isEmpty()) {
        return {};
    }
    QVector<EntryId> result;
    result.reserve(m_playback_queue.size());

    // Just copy, do not use reference
    QVector<Node*> groups = m_root->children;

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(groups.begin(), groups.end(), g);
    for (Node* group : groups) {
        for (Node* track_node : group->children) {
            if (!track_node->id.is_null()) {
                result.append(track_node->id);
            }
        }
    }
    logger->info("(re)build group_shuffle_queue");
    return result;
}

QVector<EntryId> PlaylistViewModel::generate_single_shuffle_queue()
{
    if (!m_root || m_root->children.isEmpty()) {
        return {};
    }
    QVector<EntryId> result;
    result.reserve(m_playback_queue.size());
    result = m_playback_queue;

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(result.begin(), result.end(), g);
    m_single_shuffle_queue.clear();
    m_single_shuffle_queue = result;
    logger->info("(re)build single_shuffle_queue");
    return result;
}

/* ==== Dynamic Column Management ==== */

void PlaylistViewModel::insert_column(int index, const TableColumn& column)
{
    if (index < 0 || index > m_columns.size())
        return;
    beginInsertColumns(QModelIndex(), index, index);
    m_columns.insert(index, column);
    endInsertColumns();
}

void PlaylistViewModel::remove_column(int index)
{
    if (index < 0 || index >= m_columns.size())
        return;
    beginRemoveColumns(QModelIndex(), index, index);
    m_columns.removeAt(index);
    endRemoveColumns();
}

void PlaylistViewModel::set_columns(const QVector<TableColumn>& columns)
{
    beginResetModel();
    m_columns = columns;
    endResetModel();
}

const QVector<TableColumn>& PlaylistViewModel::get_columns() const
{
    return m_columns;
}

void PlaylistViewModel::set_group_rules(const QVector<SortRule>& rules)
{
    m_layout_builder.set_group_rule(rules);
}

void PlaylistViewModel::set_sort_rules(const QVector<SortRule>& rules)
{
    m_layout_builder.set_sort_rule(rules);
}

const QVector<SortRule> PlaylistViewModel::group_rules() const
{
    return m_layout_builder.group_rules();
}

const QVector<SortRule> PlaylistViewModel::sort_rules() const
{
    return m_layout_builder.sort_rules();
}
