#pragma once

#include "core/types.h"
#include "model/playlist/playlist.h"
#include "model/playlist/playlist_layout.h"
#include "model/playlist/playlist_repo.h"

#include <QAbstractTableModel>
#include <QHash>
#include <QMimeData>
#include <QModelIndex>
#include <QPersistentModelIndex>
#include <QStringList>
#include <QTimer>
#include <QUuid>
#include <QVariant>
#include <QVector>

class PlaylistViewModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit PlaylistViewModel(PlaylistRepo* repo, QObject* parent = nullptr);
    PlaylistViewModel(const PlaylistViewModel&)            = delete;
    PlaylistViewModel& operator=(const PlaylistViewModel&) = delete;
    PlaylistViewModel(PlaylistViewModel&&)                 = delete;
    PlaylistViewModel& operator=(PlaylistViewModel&&)      = delete;
    ~PlaylistViewModel() override;

    void rebuild();
    void rebuild_async();

    /// 内容级刷新: 仅同步这些条目的 missing 状态并以 dataChanged 通知,
    /// 不 reset 模型 → 滚动位置/展开状态保持不变(用于库刷新的缺失变化)。
    void apply_missing_changes(const QVector<EntryId>& ids);

    /// 行级移除: 精确删除这些条目并以 beginRemoveRows/endRemoveRows 通知,
    /// 组标题计数同步刷新, 空组自动删除。
    /// 不 reset 模型 → 用于"清理缺失"等操作时列表不跳顶。
    void remove_tracks_by_ids(const QVector<EntryId>& ids);

    /* ==== Context & Repo 绑定 ==== */
    void set_playlist(const PlaylistId& pid);
    void set_sort_expression(const QString& expression);
    void set_group_rules(const QVector<SortRule>& rules);
    void set_sort_rules(const QVector<SortRule>& rules);
    [[nodiscard]] QVector<SortRule> group_rules() const;
    [[nodiscard]] QVector<SortRule> sort_rules() const;
    // 最近一次 DSL 解析/校验错误(空 = 无错误)
    [[nodiscard]] const QString& dsl_error() const
    {
        return layout_builder_.dsl_error();
    }
    // 当前生效的 DSL 源文本(空 = 未启用 DSL)
    [[nodiscard]] const QString& dsl_source() const
    {
        return layout_builder_.dsl_source();
    }

    /**
     * @attention default group rule = title or filename if title does not exist
     */
    void set_single_grouping(SortRule rule);
    void set_active_track(const EntryId& tid);
    void clear();

    /* ==== View视图数据访问 ====*/
    // QAbstractItemModel Interface
    [[nodiscard]] QModelIndex index(int row, int column,
                                    const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QModelIndex parent(const QModelIndex& child) const override;
    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index,
                                int role = Qt::DisplayRole) const override;
    [[nodiscard]] QVariant headerData(int section, Qt::Orientation orientation,
                                      int role) const override;

    /**
     * @brief Inherited from QAbstractItemModel, When header is clicked, change the sort
     *        state and rebuild table view automatically
     * @todo map column to SortType
     */
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    // 拖拽源:列表→列表(携带源列表 id 与选中条目 id,JSON 序列化)
    [[nodiscard]] Qt::ItemFlags flags(const QModelIndex& index) const override;
    [[nodiscard]] QStringList mimeTypes() const override;
    [[nodiscard]] QMimeData* mimeData(const QModelIndexList& indexes) const override;

    // Helper to get logic data
    [[nodiscard]] PlaybackQueueSnapshot
    playback_queue_snapshot() const; // return value but not ptr because use rebuild_async()
    [[nodiscard]] PlaybackQueueSnapshot single_shuffle_queue_snapshot() const;
    [[nodiscard]] PlaybackQueueSnapshot group_shuffle_queue_snapshot() const;
    [[nodiscard]] EntryId track_at(int index) const; // Still useful for linear queue access
    [[nodiscard]] EntryId track_at(const QModelIndex& index) const;
    [[nodiscard]] QModelIndex get_current_track_index();
    [[nodiscard]] const QVector<EntryId>& playback_queue() const;

    const Playlist& resolve_playlist();

    /* ==== Dynamic Column Management ==== */
    void insert_column(int index, const TableColumn& column);
    void remove_column(int index);
    void rename_column(int index, const QString& new_name);
    void set_columns(const QVector<TableColumn>& columns);
    [[nodiscard]] const QVector<TableColumn>& get_columns() const;

    /* ==== 播放顺序辅助（用于Player） ==== */
    [[nodiscard]] QVector<EntryId> generate_group_shuffle_queue();
    [[nodiscard]] QVector<EntryId> generate_single_shuffle_queue();

private:
    QVector<TableColumn> columns_;
    void init_default_columns();

    [[nodiscard]] QModelIndex find_track_index(const EntryId& tid) const;
    // 由 Node 反向构造 QModelIndex(与 index() 保持一致)
    QModelIndex index_of_node(const Node* node) const;
    QPersistentModelIndex active_track_index_;

    void schedule_batch_rebuild();

signals:
    void playback_queue_changed();
    void track_metadata_updated(const EntryId& tid);
    void data_changed(int row);

private:
    PlaylistRepo* repo_ = nullptr;
    PlaylistId pid_;
    EntryId active_track_id_;
    Node* root_                  = nullptr;

    int rebuild_token_           = 0;

    QTimer* batch_rebuild_timer_ = nullptr;

    QVector<EntryId> playback_queue_; // 顺序播放列表 (平铺, 与树状列表不同)
    QVector<EntryId> single_shuffle_queue_;
    QVector<EntryId> group_shuffle_queue_;

    PlayMode play_mode_ = PlayMode::InOrder;

    PlaylistLayoutBuilder layout_builder_;
};
