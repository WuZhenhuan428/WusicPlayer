#pragma once

#include <QAbstractTableModel>
#include <QVector>
#include <QUuid>
#include <QHash>
#include <QVariant>
#include <QModelIndex>
#include <QStringList>
#include <QTimer>

#include "playlist.h"
#include "playlist_repo.h"
#include "playlist_definitions.h"
#include "playlist_layout.h"

using trackId = QUuid;
using playlistId = QUuid;

class PlaylistViewModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit PlaylistViewModel(PlaylistRepo* repo, QObject* parent = nullptr);
    ~PlaylistViewModel();

    void rebuild();
    void rebuildAsync();

/* ==== Context & Repo 绑定 ==== */
    void setPlaylist(const playlistId& playlist_id);

    /**
     * @brief Parse the DSL used to set the sorting rules
     * @details format: `%key1% %key2% | %key3% %key4% ...`
     */
    void setSortExpression(const QString& expression);

    /**
     * @attention default group rule = title or filename if title does not exist
     */
    void setSingleGrouping(SortRule rule);
    void setActiveTrack(const trackId& track_id);
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
    trackId trackAt(int index) const; // Still useful for linear queue access
    trackId trackAt(const QModelIndex& index) const;
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
    void setPlayMode(PlayMode to_mode);
    trackId nextOf(const trackId& track_id) const;
    trackId previousOf(const trackId& track_id) const;

/* ==== 元数据请求 ==== */
    void requestMetaData(const trackId& track_id);

private:
   QVector<TableColumn> m_columns;
   void initDefaultColumns();

    void scheduleBatchRebuild();

signals:
    void changedPlaybackQueue();
    void updatedTrackMetadata(const trackId& track_id);
    void changedData(int row);
    
private:
    PlaylistRepo* m_repo = nullptr;
    playlistId m_playlistId;
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
