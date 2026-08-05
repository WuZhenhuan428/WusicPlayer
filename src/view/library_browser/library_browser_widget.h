#pragma once

#include "core/config_manager/i_configurable.h"
#include "core/types.h"
#include "model/library/library_browse_model.h"

#include <QTimer>
#include <QWidget>

#include <functional>

class LibraryManager;
class PlaybackQueueService;
class QComboBox;
class QLineEdit;
class QMenu;
class QModelIndex;
class QPushButton;
class QTreeView;

/**
 * @brief 媒体库浏览控件:分类浏览 + 实时 FTS5 搜索 + 入队即播。
 *
 * 布局(自上而下):
 *   [分类 ComboBox] [设置(DSL 后续,禁用)] [配置→设置面板-媒体库]
 *   [搜索 QLineEdit(防抖)]
 *   [QTreeView 分组树(分组节点默认折叠)]
 *
 * 双击曲目 → 经 PlaybackQueueService 入队即播(未注入时发 sgnPlayRequested 兜底);
 * 双击分组节点 → 展开/折叠。
 *
 * 右键 Item:播放 / 添加到播放列表 / 刷新(分组节点与曲目节点统一;不提供背景右键)。
 * 拖拽源:曲目行携带库曲目 TrackId;分组行展开为该组全部曲目;多选混合节点为无效拖拽。
 *
 * 依赖注入均非拥有、可空:set_library_manager / set_playback_queue_service。
 *
 * 持久化(config_sub_key = "library_browser"):分类、关键字、树表头状态。
 */
class LibraryBrowserWidget : public QWidget, public IConfigurable
{
    Q_OBJECT
public:
    explicit LibraryBrowserWidget(QWidget* parent = nullptr);

    void set_library_manager(LibraryManager* lib);
    void set_playback_queue_service(PlaybackQueueService* svc);

    LibraryBrowseModel* model() const
    {
        return m_model;
    }

    // 需要目标播放列表(右键子菜单用):返回 (id, name) 列表(含当前列表)
    using PlaylistListProvider = std::function<QVector<QPair<PlaylistId, QString>>()>;
    void set_playlist_list_provider(PlaylistListProvider provider);

    // config S/L interface
    void load_from_json(const QJsonObject& json) override;
    QJsonObject save_to_json() override;
    QString config_sub_key() const override;

signals:
    // 双击库曲目请求播放(PlaybackQueueService 未注入时的兜底路径)
    void sgnPlayRequested(const TrackId& track_id);
    // 播放一组库曲目(右键"播放"批量)
    void sgnPlayTracksRequested(QVector<TrackId> track_ids);
    // 把库曲目复制到目标播放列表(右键"Add to Playlist")
    void sgnAddTracksToPlaylist(PlaylistId dst_pid, QVector<TrackId> track_ids);
    // 右键菜单: 刷新媒体库
    void sgnRefreshLibraryRequested();
    // 配置按钮:跳转设置面板 "Media Library" 页
    void sgnOpenLibrarySettingsRequested();

private:
    void init_ui();
    void init_connections();
    void on_grouping_changed(int index);
    void on_keyword_changed(const QString& keyword);
    void on_double_clicked(const QModelIndex& index);
    void call_tree_context_menu(const QPoint& pos);

    QComboBox* m_cb_grouping    = nullptr;
    QPushButton* m_btn_settings = nullptr;
    QPushButton* m_btn_config   = nullptr;
    QLineEdit* m_le_keyword     = nullptr;
    QTreeView* m_tree           = nullptr;
    LibraryBrowseModel* m_model = nullptr;
    QTimer* m_tim_input         = nullptr;
    PlaylistListProvider m_playlist_provider;

    LibraryManager* m_lib             = nullptr; // 非拥有
    PlaybackQueueService* m_queue_svc = nullptr; // 非拥有
};
