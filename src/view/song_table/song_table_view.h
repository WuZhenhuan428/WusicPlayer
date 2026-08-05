#pragma once

#include "core/config_manager/i_configurable.h"
#include "core/types.h"

#include <QTreeView>
#include <QVector>
#include <QWidget>

#include <functional>

class QAbstractItemModel;
class QHeaderView;
class QMenu;
class QModelIndex;

// 支持接收媒体库拖入(添加到当前播放列表)的歌曲表
class SongTableDropView : public QTreeView
{
    Q_OBJECT
public:
    explicit SongTableDropView(QWidget* parent = nullptr);

signals:
    void sgnLibraryTracksDropped(QVector<TrackId> track_ids);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
};

/**
 * @brief 当前播放列表歌曲表:QTreeView + PlaylistViewModel 的独立控件。
 *
 * - 双击曲目 → sgnPlayTrackByModelIndex
 * - 行右键:单选(播放/移除/属性/Add to Playlist)与多选(移除/Add to Playlist)菜单
 * - 表头右键:插入 / 删除列
 * - modelReset 时自动展开分组节点(第一列 span,带展开箭头)
 * - 表头状态持久化(config_sub_key = "song_table_view")
 */
class SongTableView : public QWidget, public IConfigurable
{
    Q_OBJECT
public:
    explicit SongTableView(QWidget* parent = nullptr);

    void setModel(QAbstractItemModel* model);
    QTreeView* tree_view() const;
    QHeaderView* tree_header() const;

    // 提供"Add to Playlist"目标列表(id, 名称);返回当前列表 id 以便排除
    using PlaylistListProvider = std::function<QVector<QPair<PlaylistId, QString>>()>;
    void set_playlist_list_provider(PlaylistListProvider provider);

    // config S/L interface
    void load_from_json(const QJsonObject& json) override;
    QJsonObject save_to_json() override;
    QString config_sub_key() const override;

signals:
    void sgnPlayTrackByModelIndex(const QModelIndex& index);
    void sgnTrackPropertyRequested(EntryId tid, QString filepath, TrackMetaData meta);
    void sgnRemoveTrackRequested(EntryId tid);
    void sgnRemoveMissingTracksRequested();
    // 批量移除(多选;自动过滤组节点)
    void sgnRemoveTracksRequested(QVector<EntryId> track_ids);
    // 复制选中曲目到目标播放列表
    void sgnCopyTracksToPlaylist(PlaylistId dst_pid, QVector<EntryId> track_ids);
    // 媒体库曲目拖入 → 添加到当前播放列表
    void sgnLibraryTracksDropped(QVector<TrackId> track_ids);

private:
    void init_ui();
    void init_connections();
    void update_song_view();
    void call_song_context_menu(const QPoint& pos);
    void show_header_context_menu(const QPoint& pos);
    void build_add_to_playlist_menu(QMenu* menu, const QVector<EntryId>& track_ids);

    SongTableDropView* m_tree_view = nullptr;
    QHeaderView* m_tree_header     = nullptr;
    PlaylistListProvider m_playlist_provider;
};
