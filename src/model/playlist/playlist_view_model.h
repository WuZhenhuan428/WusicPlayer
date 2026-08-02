#pragma once

#include "core/types.h"
#include "playlist.h"
#include "playlist_layout.h"
#include "playlist_repo.h"

#include <QAbstractTableModel>
#include <QHash>
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
    void rebuildAsync();

    /* ==== Context & Repo 绑定 ==== */
    void setPlaylist(const PlaylistId& pid);
    void setSortExpression(const QString& expression);
    void setGroupRules(const QVector<SortRule>& rules);
    void setSortRules(const QVector<SortRule>& rules);
    const QVector<SortRule> groupRules() const;
    const QVector<SortRule> sortRules() const;

    /**
     * @attention default group rule = title or filename if title does not exist
     */
    void setSingleGrouping(SortRule rule);
    void setActiveTrack(const EntryId& tid);
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

    // Helper to get logic data
    PlaybackQueueSnapshot
    playbackQueueSnapshot() const; // return value but not ptr because use rebuildAsync()
    PlaybackQueueSnapshot singleShuffleQueueSnapshot() const;
    PlaybackQueueSnapshot groupShuffleQueueSnapshot() const;
    EntryId trackAt(int index) const; // Still useful for linear queue access
    EntryId trackAt(const QModelIndex& index) const;
    QModelIndex getCurrentTrackIndex();
    const QVector<EntryId>& playbackQueue() const;

    const Playlist& resolvePlaylist();

    /* ==== Dynamic Column Management ==== */
    void insertColumn(int index, const TableColumn& column);
    void removeColumn(int index);
    void setColumns(const QVector<TableColumn>& columns);
    const QVector<TableColumn>& getColumns() const;

    /* ==== 播放顺序辅助（用于Player） ==== */
    QVector<EntryId> generateGroupShuffleQueue();
    QVector<EntryId> generateSingleShuffleQueue();

private:
    QVector<TableColumn> m_columns;
    void initDefaultColumns();

    QModelIndex findTrackIndex(const EntryId& tid) const;
    QPersistentModelIndex m_active_track_index;

    void scheduleBatchRebuild();

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
