#pragma once

#include <QAbstractTableModel>
#include <QVector>
#include <QUuid>
#include <QHash>
#include <QVariant>
#include <QModelIndex>
#include <QPersistentModelIndex>
#include <QStringList>
#include <QTimer>

#include "playlist.h"
#include "playlist_repo.h"
#include "core/types.h"
#include "playlist_layout.h"

class PlaylistViewModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit PlaylistViewModel(PlaylistRepo* repo, QObject* parent = nullptr);
    ~PlaylistViewModel();

    void rebuild();
    void rebuildAsync();

/* ==== Context & Repo 绑定 ==== */
    void setPlaylist(const playlistId& pid);

    /**
     * @brief Parse the DSL used to set the sorting rules
     * @details format: `%key1% %key2% | %key3% %key4% ...`
     */
    void setSortExpression(const QString& expression);
    void setGroupRules(const QVector<SortRule>& rules);
    void setSortRules(const QVector<SortRule>& rules);
    const QVector<SortRule> groupRules() const;
    const QVector<SortRule> SortRules() const;

    /**
     * @attention default group rule = title or filename if title does not exist
     */
    void setSingleGrouping(SortRule rule);
    void setActiveTrack(const trackId& tid);
    void clear();

/* ==== View视图数据访问 ====*/
    // QAbstractItemModel Interface
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

    /**
     * @brief Inherited from QAbstractItemModel, When header is clicked, change the sort
     *        state and rebuild table view automatically
     * @todo map column to SortType
     */
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

    // Helper to get logic data
    PlaybackQueueSnapshot playbackQueueSnapshot() const; // return value but not ptr because use rebuildAsync()
    PlaybackQueueSnapshot singleShuffleQueueSnapshot() const;
    PlaybackQueueSnapshot groupShuffleQueueSnapshot() const;
    trackId trackAt(int index) const; // Still useful for linear queue access
    trackId trackAt(const QModelIndex& index) const;
    QModelIndex getCurrentTrackIndex();
    const QVector<trackId>& playbackQueue() const;

    const Playlist& resolvePlaylist();

/* ==== Dynamic Column Management ==== */
    void insertColumn(int index, const TableColumn& column);
    void removeColumn(int index);
    void setColumns(const QVector<TableColumn>& columns);
    const QVector<TableColumn>& getColumns() const;

/* ==== 播放顺序辅助（用于Player） ==== */
    QVector<trackId> generateGroupShuffleQueue();
    QVector<trackId> generateSingleShuffleQueue();

private:
    QVector<TableColumn> m_columns;
    void initDefaultColumns();

    QModelIndex findTrackIndex(const trackId& tid) const;
    QPersistentModelIndex m_activeTrackIndex;

    void scheduleBatchRebuild();

signals:
    void changedPlaybackQueue();
    void updatedTrackMetadata(const trackId& tid);
    void changedData(int row);
    
private:
    PlaylistRepo* m_repo = nullptr;
    playlistId m_pid;
    trackId m_activeTrackId;
    Node* m_root = nullptr;

    int m_rebuildToken = 0;

    QTimer* m_batchRebuildTimer = nullptr;

    QVector<trackId> m_playbackQueue; // Linear queue for playback logic (separate from Tree structure)
    QVector<trackId> m_singleShuffleQueue;
    QVector<trackId> m_groupShuffleQueue;

    PlayMode m_playMode;

    PlaylistLayoutBuilder m_layoutBuilder;
};
