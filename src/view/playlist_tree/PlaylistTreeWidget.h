#pragma once

#include "core/types.h"

#include <QWidget>

class QPoint;
class QTreeWidget;
class QTreeWidgetItem;

/**
 * @brief 播放列表导航控件:播放列表树 + 双击切换 + CRUD 右键。
 *
 * - 双击列表项 → sgnSwitchPlaylist
 * - 列表项右键:添加曲目/文件夹、保存、重命名、复制、删除
 * - 由外部(setPlaylists)填充播放列表
 */
class PlaylistTreeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PlaylistTreeWidget(QWidget* parent = nullptr);

    void setPlaylists(const QVector<QPair<PlaylistId, QString>>& playlists);

signals:
    void sgnSwitchPlaylist(const PlaylistId& id);
    void sgnImportFiles(const PlaylistId& pid = PlaylistId());
    void sgnImportDir(const PlaylistId& pid = PlaylistId());
    void sgnSavePlaylist(const PlaylistId& id);
    void sgnRenamePlaylist(const PlaylistId& id);
    void sgnCopyPlaylist(const PlaylistId& id);
    void sgnRemovePlaylist(const PlaylistId& id);

private:
    void initUI();
    void initConnections();
    void callTreeContextMenu(const QPoint& pos);

    QTreeWidget* m_playlist_tree = nullptr;
};
