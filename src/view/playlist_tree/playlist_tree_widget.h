#pragma once

#include "core/types.h"

#include <QTreeWidget>
#include <QWidget>

class QPoint;
class QTreeWidgetItem;

// 支持接收媒体库/播放列表条目拖入的播放列表树
class PlaylistTreeDropView : public QTreeWidget
{
    Q_OBJECT
public:
    explicit PlaylistTreeDropView(QWidget* parent = nullptr);

signals:
    // 媒体库曲目拖放到某播放列表项上(背景不发送)
    void sgnLibraryTracksDropped(PlaylistId pid, QVector<TrackId> track_ids);
    // 播放列表条目(列表→列表)拖放到某播放列表项上(背景不发送)
    void sgnPlaylistEntriesDropped(PlaylistId src_pid, PlaylistId dst_pid,
                                   QVector<EntryId> entry_ids);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
};

/**
 * @brief 播放列表导航控件:播放列表树 + 双击切换 + CRUD 右键。
 *
 * - 单击选中;再次单击已选中项(SelectedClicked)进入内联重命名;双击切换列表
 * - 列表项右键:新建/添加曲目/文件夹、保存、重命名(内联)、复制并粘贴、删除
 * - 背景右键:新建播放列表
 * - 拖动(单选):重排播放列表顺序
 * - 由外部(set_playlists)填充播放列表
 */
class PlaylistTreeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlaylistTreeWidget(QWidget* parent = nullptr);

    void set_playlists(const QVector<QPair<PlaylistId, QString>>& playlists);

signals:
    void sgnSwitchPlaylist(const PlaylistId& id);
    void sgnCreatePlaylist();
    void sgnImportFiles(const PlaylistId& pid = PlaylistId());
    void sgnImportDir(const PlaylistId& pid = PlaylistId());
    void sgnSavePlaylist(const PlaylistId& id);
    // 内联重命名提交(编辑完成时带新名字)
    void sgnRenamePlaylist(const PlaylistId& id, const QString& new_name);
    void sgnCopyPlaylist(const PlaylistId& id);
    void sgnRemovePlaylist(const PlaylistId& id);
    // 拖动排序完成后,按新顺序提交
    void sgnReorderPlaylists(const QVector<PlaylistId>& ordered_ids);
    // 媒体库曲目拖入某播放列表项(背景不发送)
    void sgnLibraryTracksDropped(PlaylistId pid, QVector<TrackId> track_ids);
    // 播放列表条目拖入某播放列表项(列表→列表;背景不发送)
    void sgnPlaylistEntriesDropped(PlaylistId src_pid, PlaylistId dst_pid,
                                   QVector<EntryId> entry_ids);

private:
    void init_ui();
    void init_connections();
    void call_tree_context_menu(const QPoint& pos);
    void on_item_double_clicked(QTreeWidgetItem* item);
    void on_item_changed(QTreeWidgetItem* item);
    void begin_rename(QTreeWidgetItem* item);
    void on_rows_moved();
    QVector<PlaylistId> collect_order() const;

    PlaylistTreeDropView* m_playlist_tree = nullptr;
};
