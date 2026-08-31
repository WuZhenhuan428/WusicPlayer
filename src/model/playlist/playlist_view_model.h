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
    ~PlaylistViewModel();

    void rebuild();
    void rebuild_async();

    /* ==== Context & Repo 绑定 ==== */
    void set_playlist(const PlaylistId& pid);
    void set_sort_expression(const QString& expression);
    void set_group_rules(const QVector<SortRule>& rules);
    void set_sort_rules(const QVector<SortRule>& rules);
    const QVector<SortRule> group_rules() const;
    const QVector<SortRule> sort_rules() const;
    // 最近一次 DSL 解析/校验错误(空 = 无错误)
    const QString& dsl_error() const
    {
        return m_layout_builder.dsl_error();
    }
    // 当前生效的 DSL 源文本(空 = 未启用 DSL)
    const QString& dsl_source() const
    {
        return m_layout_builder.dsl_source();
    }

    /**
     * @attention default group rule = title or filename if title does not exist
     */
    void set_single_grouping(SortRule rule);
    void set_active_track(const EntryId& tid);
    void clear();

    /* ==== View视图数据访问 ====*/
    // QAbstractItemModel Interface
    QModelIndex index(int row, int column,
                      const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& child) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    /**
     * @brief Inherited from QAbstractItemModel, When header is clicked, change the sort
     *        state and rebuild table view automatically
     * @todo map column to SortType
     */
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    // 拖拽源:列表→列表(携带源列表 id 与选中条目 id,JSON 序列化)
    Qt::ItemFlags flags(const QModelIndex& index) const override;
    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;

    // Helper to get logic data
    PlaybackQueueSnapshot
    playback_queue_snapshot() const; // return value but not ptr because use rebuild_async()
    PlaybackQueueSnapshot single_shuffle_queue_snapshot() const;
    PlaybackQueueSnapshot group_shuffle_queue_snapshot() const;
    EntryId track_at(int index) const; // Still useful for linear queue access
    EntryId track_at(const QModelIndex& index) const;
    QModelIndex get_current_track_index();
    const QVector<EntryId>& playback_queue() const;

    const Playlist& resolve_playlist();

    /* ==== Dynamic Column Management ==== */
    void insert_column(int index, const TableColumn& column);
    void remove_column(int index);
    void set_columns(const QVector<TableColumn>& columns);
    const QVector<TableColumn>& get_columns() const;

    /* ==== 播放顺序辅助（用于Player） ==== */
    QVector<EntryId> generate_group_shuffle_queue();
    QVector<EntryId> generate_single_shuffle_queue();

private:
    QVector<TableColumn> m_columns;
    void init_default_columns();

    QModelIndex find_track_index(const EntryId& tid) const;
    QPersistentModelIndex m_active_track_index;

    void schedule_batch_rebuild();

signals:
    void changedPlaybackQueue();
    void updatedTrackMetadata(const EntryId& tid);
    void changedData(int row);

private:
    PlaylistRepo* m_repo = nullptr;
    PlaylistId m_pid;
    EntryId m_active_track_id;
    Node* m_root                  = nullptr;

    int m_rebuild_token           = 0;

    QTimer* m_batch_rebuild_timer = nullptr;

    QVector<EntryId>
        m_playback_queue; // Linear queue for playback logic (separate from Tree structure)
    QVector<EntryId> m_single_shuffle_queue;
    QVector<EntryId> m_group_shuffle_queue;

    PlayMode m_play_mode;

    PlaylistLayoutBuilder m_layout_builder;
};
