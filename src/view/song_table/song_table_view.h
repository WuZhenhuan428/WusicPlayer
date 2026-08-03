#pragma once

#include "core/config_manager/i_configurable.h"
#include "core/types.h"

#include <QWidget>

class QAbstractItemModel;
class QHeaderView;
class QModelIndex;
class QTreeView;

/**
 * @brief 当前播放列表歌曲表:QTreeView + PlaylistViewModel 的独立控件。
 *
 * - 双击曲目 → sgnPlayTrackByModelIndex
 * - 行右键:播放 / 移除 / 移除缺失 / 打开所在文件夹 / 属性
 * - 表头右键:插入 / 删除列
 * - modelReset 时自动展开分组节点(第一列 span)
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

    // config S/L interface
    void load_from_json(const QJsonObject& json) override;
    QJsonObject save_to_json() override;
    QString config_sub_key() const override;

signals:
    void sgnPlayTrackByModelIndex(const QModelIndex& index);
    void sgnTrackPropertyRequested(EntryId tid, QString filepath, TrackMetaData meta);
    void sgnRemoveTrackRequested(EntryId tid);
    void sgnRemoveMissingTracksRequested();

private:
    void init_ui();
    void init_connections();
    void update_song_view();
    void call_song_context_menu(const QPoint& pos);
    void show_header_context_menu(const QPoint& pos);

    QTreeView* m_tree_view     = nullptr;
    QHeaderView* m_tree_header = nullptr;
};
