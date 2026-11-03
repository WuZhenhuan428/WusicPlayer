#include "model/playlist/playlist_view_model.h"

#include <QColor>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMimeData>
#include <QPointer>
#include <QRegularExpression>
#include <QSet>
#include <QThread>
#include <QTime>

#include <functional>
#include <random>
#include <ranges>

#include "core/logger/logger_manager.h"
namespace
{
// NOLINTNEXTLINE
Logger* logger = LoggerManager::file_logger("playlist_view_model", {"console", "gui"});
} // namespace

PlaylistViewModel::PlaylistViewModel(PlaylistRepo* repo, QObject* parent) :
    QAbstractItemModel(parent), repo_(repo)
{
    init_default_columns();
    root_                = new Node();
    batch_rebuild_timer_ = new QTimer(this);
    batch_rebuild_timer_->setSingleShot(true);
    batch_rebuild_timer_->setInterval(60);
    connect(batch_rebuild_timer_, &QTimer::timeout, this, &PlaylistViewModel::rebuild_async);
    if (repo_) {
        connect(repo_, &PlaylistRepo::sgn_playlist_changed, this,
                &PlaylistViewModel::rebuild_async);
        connect(repo_, &PlaylistRepo::playlist_batch_loaded, this,
                [this](const PlaylistId& playlist_id, int, int) {
                    if (playlist_id == pid_) {
                        schedule_batch_rebuild();
                    }
                });
    }
}

void PlaylistViewModel::init_default_columns()
{
    columns_ = {
        {"", SortType::NotSorted},    {"Disc", SortType::DiscNumber},
        {"#", SortType::TrackNumber}, {"Title", SortType::Title},
        {"Artist", SortType::Artist}, {"Duration", SortType::Duration},
        {"Album", SortType::Album},
    };
}

PlaylistViewModel::~PlaylistViewModel()
{
    delete root_;
}

void PlaylistViewModel::schedule_batch_rebuild()
{
    if (!batch_rebuild_timer_) {
        return;
    }
    if (!batch_rebuild_timer_->isActive()) {
        batch_rebuild_timer_->start();
    }
}

const Playlist& PlaylistViewModel::resolve_playlist()
{
    // Warning: This might crash if playlist is not found!
    // Prefer using find_playlist_by_id directly with check.
    return *(repo_->find_playlist_by_id(pid_));
}

void PlaylistViewModel::rebuild()
{
    beginResetModel();

    // Clean old data
    delete root_;

    if (!repo_) {
        root_ = new Node();
        playback_queue_.clear();
        endResetModel();
        active_track_index_ = QPersistentModelIndex(find_track_index(active_track_id_));
        return;
    }

    // Fix: Check pointer validity to avoid Core Dump
    auto playlist_ptr = repo_->find_playlist_by_id(pid_);
    if (!playlist_ptr) {
        root_ = new Node();
        playback_queue_.clear();
        endResetModel();
        active_track_index_ = QPersistentModelIndex(find_track_index(active_track_id_));
        return;
    }

    const Playlist& pl  = *playlist_ptr;

    LayoutResult layout = layout_builder_.build(pl);

    if (!layout.updated_meta.isEmpty()) {
        for (const auto& entry : layout.updated_meta) {
            playlist_ptr->update_track_meta(entry.id, entry.meta);
        }
        repo_->save_list_to_cache(playlist_ptr);
    }

    root_                 = layout.root;
    playback_queue_       = layout.playback_queue;
    single_shuffle_queue_ = generate_single_shuffle_queue();
    group_shuffle_queue_  = generate_group_shuffle_queue();

    endResetModel();
    active_track_index_ = QPersistentModelIndex(find_track_index(active_track_id_));
    logger->info("rebuild finished. Queue size: {}", playback_queue_.size());
    emit playback_queue_changed();
}

void PlaylistViewModel::rebuild_async()
{
    const int token = ++rebuild_token_;

    if (!repo_) {
        beginResetModel();
        delete root_;
        root_ = new Node();
        playback_queue_.clear();
        endResetModel();
        active_track_index_ = QPersistentModelIndex(find_track_index(active_track_id_));
        return;
    }

    auto playlist_ptr = repo_->find_playlist_by_id(pid_);
    if (!playlist_ptr) {
        beginResetModel();
        delete root_;
        root_ = new Node();
        playback_queue_.clear();
        endResetModel();
        active_track_index_ = QPersistentModelIndex(find_track_index(active_track_id_));
        return;
    }

    auto playlist_snapshot             = std::make_shared<Playlist>(*playlist_ptr);
    PlaylistLayoutBuilder builder_copy = layout_builder_;

    QPointer<PlaylistViewModel> self(this);
    QThread* worker = QThread::create([self, token, playlist_snapshot, builder_copy]() mutable {
        if (!self) {
            return;
        }

        LayoutResult layout = builder_copy.build(*playlist_snapshot);
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
                if (token != self->rebuild_token_) {
                    delete layout.root;
                    return;
                }

                self->beginResetModel();
                delete self->root_;
                self->root_                 = layout.root;
                self->playback_queue_       = layout.playback_queue;
                self->single_shuffle_queue_ = self->generate_single_shuffle_queue();
                self->group_shuffle_queue_  = self->generate_group_shuffle_queue();
                self->endResetModel();
                self->active_track_index_ =
                    QPersistentModelIndex(self->find_track_index(self->active_track_id_));

                auto playlist_ptr =
                    self->repo_ ? self->repo_->find_playlist_by_id(self->pid_) : nullptr;
                if (playlist_ptr && !layout.updated_meta.isEmpty()) {
                    for (const auto& entry : layout.updated_meta) {
                        playlist_ptr->update_track_meta(entry.id, entry.meta);
                    }
                    self->repo_->save_list_to_cache(playlist_ptr);
                }
                emit self->playback_queue_changed();
            },
            Qt::QueuedConnection);
    });

    QObject::connect(worker, &QThread::finished, worker, &QObject::deleteLater);
    worker->start();
}

void PlaylistViewModel::apply_missing_changes(const QVector<EntryId>& ids)
{
    if (!repo_ || !root_ || ids.isEmpty()) {
        return;
    }
    auto playlist = repo_->find_playlist_by_id(pid_);
    if (!playlist) {
        return;
    }

    // 预取目标缺失状态(键用稳定字符串, 避免逐条遍历树)
    QHash<QString, bool> target;
    target.reserve(ids.size());
    for (const EntryId& id : ids) {
        const Track* t = playlist->find_track_by_id(id);
        if (t) {
            target.insert(id.to_string_without_brace(), t->missing);
        }
    }
    if (target.isEmpty()) {
        return;
    }

    // 递归遍历(支持多级分组), 只发出 dataChanged
    std::function<void(Node*, const QModelIndex&)> walk = [&](Node* node,
                                                              const QModelIndex& parent) {
        for (int row = 0; row < node->children.size(); ++row) {
            Node* child          = node->children.at(row);
            const QModelIndex ix = index(row, 0, parent);
            if (!ix.isValid()) {
                continue;
            }
            if (child->id.is_null()) {
                walk(child, ix); // 组节点
                continue;
            }
            const auto it = target.constFind(child->id.to_string_without_brace());
            if (it == target.constEnd() || child->missing == it.value()) {
                continue;
            }
            child->missing = it.value();
            emit dataChanged(ix, ix.siblingAtColumn(columnCount() - 1), {Qt::ForegroundRole});
        }
    };
    walk(root_, QModelIndex());
}

void PlaylistViewModel::remove_tracks_by_ids(const QVector<EntryId>& ids)
{
    if (!root_ || ids.isEmpty()) {
        return;
    }

    QSet<QString> want;
    want.reserve(ids.size());
    for (const EntryId& id : ids) {
        want.insert(id.to_string_without_brace());
    }

    // 1) 收集命中条目(支持多级分组)
    struct Hit
    {
        Node* parent = nullptr;
        int row      = 0;
        Node* node   = nullptr;
    };
    QVector<Hit> hits;
    std::function<void(Node*)> collect = [&](Node* parent) {
        for (int row = 0; row < parent->children.size(); ++row) {
            Node* child = parent->children.at(row);
            if (child->id.is_null()) {
                collect(child); // 组节点
                continue;
            }
            if (want.contains(child->id.to_string_without_brace())) {
                hits.append({parent, row, child});
            }
        }
    };
    collect(root_);
    if (hits.isEmpty()) {
        return;
    }

    // 同一父节点内从大行号开始删除, 行号不因删除位移
    std::ranges::sort(hits, [](const Hit& a, const Hit& b) {
        if (a.parent != b.parent) {
            return a.parent > b.parent;
        }
        return a.row > b.row;
    });

    // 2) 逐行删除并同步各播放队列
    QVector<Node*> touched_groups; // 受影响的分组(用于刷新计数/删除空组)
    for (const Hit& hit : hits) {
        const QModelIndex parent_idx = index_of_node(hit.parent);
        beginRemoveRows(parent_idx, hit.row, hit.row);

        playback_queue_.removeAll(hit.node->id);
        single_shuffle_queue_.removeAll(hit.node->id);
        group_shuffle_queue_.removeAll(hit.node->id);

        hit.parent->children.removeAt(hit.row);
        delete hit.node;
        endRemoveRows();

        if (hit.parent != root_ && !touched_groups.contains(hit.parent)) {
            touched_groups.append(hit.parent);
        }
    }

    // 3) 空组删除(仅一级分组; 多级分组保留空子组)
    QVector<Hit> empty_groups;
    for (Node* group : touched_groups) {
        if (group->parent == root_ && group->children.isEmpty()) {
            empty_groups.append({group->parent, group->row(), group});
        }
    }
    std::ranges::sort(empty_groups, [](const Hit& a, const Hit& b) { return a.row > b.row; });
    for (const Hit& hit : empty_groups) {
        beginRemoveRows(QModelIndex(), hit.row, hit.row);
        hit.parent->children.removeAt(hit.row);
        delete hit.node;
        endRemoveRows();
    }

    // 4) 非空组刷新标题(含计数)
    for (Node* group : touched_groups) {
        if (!group->parent || group->children.isEmpty()) {
            continue;
        }
        const QModelIndex group_idx = index_of_node(group);
        if (group_idx.isValid()) {
            emit dataChanged(group_idx, group_idx);
        }
    }

    logger->info("removed {} tracks from view (no reset)", static_cast<int>(hits.size()));
    emit playback_queue_changed();
}

void PlaylistViewModel::set_playlist(const PlaylistId& pid)
{
    if (pid_ == pid) {
        return;
    }
    pid_ = pid;
    this->rebuild_async();
}

void PlaylistViewModel::set_sort_expression(const QString& expression)
{
    // DSL 路径(排序/分组/分类); 空串 → 清除 DSL, 回退内置规则
    layout_builder_.set_dsl(expression);
    this->rebuild_async();
}

void PlaylistViewModel::set_single_grouping(SortRule rule)
{
    layout_builder_.update_sort(rule, false);
    this->rebuild_async();
}

void PlaylistViewModel::set_active_track(const EntryId& tid)
{
    QModelIndex old_index = get_current_track_index();
    active_track_id_      = tid;
    active_track_index_   = QPersistentModelIndex(find_track_index(tid));

    QModelIndex new_index = get_current_track_index();

    auto emit_row_changed = [this](const QModelIndex& index) {
        if (!index.isValid()) {
            return;
        }
        const QModelIndex left  = index.siblingAtColumn(0);
        const QModelIndex right = index.siblingAtColumn(std::max(0, columnCount() - 1));
        emit dataChanged(left, right, {Qt::DisplayRole});
    };

    emit_row_changed(old_index);
    emit_row_changed(new_index);
}

void PlaylistViewModel::clear()
{
    beginResetModel();
    delete root_;
    root_ = new Node();
    playback_queue_.clear();
    endResetModel();
    active_track_index_ = QPersistentModelIndex();
}

/* ==== QAbstractItemModel Interface ==== */

QModelIndex PlaylistViewModel::index(int row, int column, const QModelIndex& parent) const
{
    if (!hasIndex(row, column, parent)) {
        return {};
    }

    Node* parent_node = nullptr;
    if (!parent.isValid()) {
        parent_node = root_;
    } else {
        parent_node = static_cast<Node*>(parent.internalPointer());
    }

    if (row < 0 || row >= parent_node->children.size()) {
        return {};
    }

    Node* child_node = parent_node->children.at(row);
    return createIndex(row, column, child_node);
}

QModelIndex PlaylistViewModel::parent(const QModelIndex& child) const
{
    if (!child.isValid()) {
        return {};
    }

    Node* child_node  = static_cast<Node*>(child.internalPointer());
    Node* parent_node = child_node->parent;

    if (parent_node == root_ || !parent_node) {
        return {};
    }

    return createIndex(parent_node->row(), 0, parent_node);
}

int PlaylistViewModel::rowCount(const QModelIndex& parent) const
{
    Node* parent_node = nullptr;
    if (parent.column() > 0) {
        return 0;
    }

    if (!parent.isValid()) {
        parent_node = root_;
    } else {
        parent_node = static_cast<Node*>(parent.internalPointer());
    }

    return static_cast<int>(parent_node->children.size());
}

int PlaylistViewModel::columnCount([[maybe_unused]] const QModelIndex& parent) const
{
    return static_cast<int>(columns_.size());
}

QVariant PlaylistViewModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    Node* node    = static_cast<Node*>(index.internalPointer());
    bool is_group = node->id.is_null();

    if (role == Qt::TextAlignmentRole && is_group) {
        return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
    }
    if (role == Qt::TextAlignmentRole && index.column() == 0) {
        return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
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
            return {};
        }

        // Track Logic
        const TrackMetaData& d = node->meta;
        if (index.column() < 0 || index.column() >= columns_.size()) {
            return {};
        }

        const TableColumn& col = columns_[index.column()];

        if (col.sort_type == SortType::NotSorted) {
            return (node->id == active_track_id_) ? ">" : "";
        }

        // Special formatting
        if (col.sort_type == SortType::Duration) {
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
        QVariant val = PlaylistLayoutBuilder::get_metadata_value(d, col.sort_type);
        if (val.isValid()) {
            return val;
        }
        return {};
    }
    return {};
}

QVariant PlaylistViewModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole) {
        return {};
    }
    if (orientation == Qt::Horizontal) {
        if (section >= 0 && section < columns_.size()) {
            return columns_[section].header_name;
        }
    }
    return {};
}

void PlaylistViewModel::sort(int column, Qt::SortOrder order)
{
    if (column < 0 || column >= columns_.size()) {
        return;
    }

    SortType type = columns_[column].sort_type;
    if (type == SortType::NotSorted) {
        return;
    }

    SortRule rule;
    rule.type  = type;
    rule.order = order;

    layout_builder_.set_sort_rule({rule});
    rebuild_async();
}

/* ==== Helpers ==== */

PlaybackQueueSnapshot PlaylistViewModel::playback_queue_snapshot() const
{
    static int version = 0;
    return {playback_queue_, version++};
}

PlaybackQueueSnapshot PlaylistViewModel::single_shuffle_queue_snapshot() const
{
    static int version = 0;
    return {single_shuffle_queue_, version++};
}

PlaybackQueueSnapshot PlaylistViewModel::group_shuffle_queue_snapshot() const
{
    static int version = 0;
    return {group_shuffle_queue_, version++};
}

EntryId PlaylistViewModel::track_at(int index) const
{
    if (index >= 0 && index < playback_queue_.size()) {
        return playback_queue_.at(index);
    }
    return {};
}

EntryId PlaylistViewModel::track_at(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return {};
    }
    Node* node = static_cast<Node*>(index.internalPointer());
    return node->id;
}

Qt::ItemFlags PlaylistViewModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags f = QAbstractItemModel::flags(index);
    if (index.isValid()) {
        // 组节点(id 为空)不参与拖拽: mimeData 只序列化曲目条目
        auto* node = static_cast<Node*>(index.internalPointer());
        if (node && !node->id.is_null()) {
            f |= Qt::ItemIsDragEnabled;
        }
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
    root["src"] = pid_.to_string_without_brace();
    root["ids"] = arr;
    auto* mime  = new QMimeData;
    mime->setData(QString::fromLatin1(wusic::kPlaylistEntriesMime),
                  QJsonDocument(root).toJson(QJsonDocument::Compact));
    return mime;
}

QModelIndex PlaylistViewModel::get_current_track_index()
{
    if (active_track_index_.isValid()) {
        return active_track_index_;
    }
    return find_track_index(active_track_id_);
}

QModelIndex PlaylistViewModel::index_of_node(const Node* node) const
{
    if (!node || !node->parent || !root_) {
        return {};
    }
    // 自顶向下沿链构造, 保证不是 O(depth) 的 parent() 反复上升
    QVector<const Node*> chain;
    for (const Node* n = node; n && n->parent; n = n->parent) {
        chain.prepend(n);
    }
    QModelIndex idx;
    for (const Node* n : chain) {
        const int row = static_cast<int>(n->parent->children.indexOf(const_cast<Node*>(n)));
        if (row < 0) {
            return {};
        }
        idx = index(row, 0, idx);
        if (!idx.isValid()) {
            return {};
        }
    }
    return idx;
}

QModelIndex PlaylistViewModel::find_track_index(const EntryId& tid) const
{
    if (tid.is_null() || !root_) {
        return {};
    }

    for (Node* group : root_->children) {
        for (int track_row = 0; track_row < group->children.size(); ++track_row) {
            Node* track_node = group->children.at(track_row);
            if (track_node->id == tid) {
                // QModelIndex parent_index = createIndex(group_row, 0, group); // unused
                return createIndex(track_row, 0, track_node);
            }
        }
    }
    return {};
}

const QVector<EntryId>& PlaylistViewModel::playback_queue() const
{
    return playback_queue_;
}

QVector<EntryId> PlaylistViewModel::generate_group_shuffle_queue()
{
    if (!root_ || root_->children.isEmpty()) {
        return {};
    }
    QVector<EntryId> result;
    result.reserve(playback_queue_.size());

    // Just copy, do not use reference
    QVector<Node*> groups = root_->children;

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
    if (!root_ || root_->children.isEmpty()) {
        return {};
    }
    QVector<EntryId> result;
    result.reserve(playback_queue_.size());
    result = playback_queue_;

    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(result.begin(), result.end(), g);
    single_shuffle_queue_.clear();
    single_shuffle_queue_ = result;
    logger->info("(re)build single_shuffle_queue");
    return result;
}

/* ==== Dynamic Column Management ==== */

void PlaylistViewModel::insert_column(int index, const TableColumn& column)
{
    if (index < 0 || index > columns_.size()) {
        return;
    }
    beginInsertColumns(QModelIndex(), index, index);
    columns_.insert(index, column);
    endInsertColumns();
}

void PlaylistViewModel::remove_column(int index)
{
    if (index < 0 || index >= columns_.size()) {
        return;
    }
    beginRemoveColumns(QModelIndex(), index, index);
    columns_.removeAt(index);
    endRemoveColumns();
}

void PlaylistViewModel::rename_column(int index, const QString& new_name)
{
    if (index < 0 || index >= columns_.size()) {
        return;
    }
    columns_[index].header_name = new_name;
}

void PlaylistViewModel::set_columns(const QVector<TableColumn>& columns)
{
    beginResetModel();
    columns_ = columns;
    endResetModel();
}

const QVector<TableColumn>& PlaylistViewModel::get_columns() const
{
    return columns_;
}

void PlaylistViewModel::set_group_rules(const QVector<SortRule>& rules)
{
    layout_builder_.set_group_rule(rules);
}

void PlaylistViewModel::set_sort_rules(const QVector<SortRule>& rules)
{
    layout_builder_.set_sort_rule(rules);
}

QVector<SortRule> PlaylistViewModel::group_rules() const
{
    return layout_builder_.group_rules();
}

QVector<SortRule> PlaylistViewModel::sort_rules() const
{
    return layout_builder_.sort_rules();
}
