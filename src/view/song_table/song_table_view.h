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
    void sgn_library_tracks_dropped(QVector<TrackId> track_ids);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
};

/**
 * @brief 当前播放列表歌曲表:QTreeView + PlaylistViewModel 的独立控件。
 *
 * - 双击曲目 → sgn_play_track_by_model_index
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

    void set_model(QAbstractItemModel* model);
    [[nodiscard]] QTreeView* tree_view() const;
    [[nodiscard]] QHeaderView* tree_header() const;

    // 提供"Add to Playlist"目标列表(id, 名称);返回当前列表 id 以便排除
    using PlaylistListProvider = std::function<QVector<QPair<PlaylistId, QString>>()>;
    void set_playlist_list_provider(PlaylistListProvider provider);

    // config S/L interface
    void load_from_json(const QJsonObject& json) override;
    QJsonObject save_to_json() override;
    [[nodiscard]] QString config_sub_key() const override;

signals:
    void sgn_play_track_by_model_index(const QModelIndex& index);
    void sgn_track_property_requested(EntryId tid, QString filepath, TrackMetaData meta);
    void sgn_remove_track_requested(EntryId tid);
    void sgn_remove_missing_tracks_requested();
    // 批量移除(多选;自动过滤组节点)
    void sgn_remove_tracks_requested(QVector<EntryId> track_ids);
    // 复制选中曲目到目标播放列表
    void sgn_copy_tracks_to_playlist(PlaylistId dst_pid, QVector<EntryId> track_ids);
    // 媒体库曲目拖入 → 添加到当前播放列表
    void sgn_library_tracks_dropped(QVector<TrackId> track_ids);

private:
    void init_ui();
    void init_connections();
    void update_song_view();
    void call_song_context_menu(const QPoint& pos);
    void show_header_context_menu(const QPoint& pos);
    void build_add_to_playlist_menu(QMenu* menu, const QVector<EntryId>& track_ids);

    SongTableDropView* tree_view_ = nullptr;
    QHeaderView* tree_header_     = nullptr;
    PlaylistListProvider playlist_provider_;
};
