#include "view/playlist_tree/playlist_tree_widget.h"

#include "view/playlist/playlist_widgets.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMimeData>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace
{
// 从 mimeData 解析库曲目 TrackId 列表;非本应用类型返回空
QVector<TrackId> decode_library_tracks(const QMimeData* mime)
{
    if (!mime) {
        return {};
    }
    const QByteArray raw = mime->data(QString::fromLatin1(wusic::kLibraryTracksMime));
    if (raw.isEmpty()) {
        return {};
    }
    QVector<TrackId> tids;
    const QJsonArray arr = QJsonDocument::fromJson(raw).array();
    for (const auto& v : arr) {
        const TrackId tid = TrackId::from_string(v.toString());
        if (!tid.is_null()) {
            tids.push_back(tid);
        }
    }
    return tids;
}

// 从 mimeData 解析播放列表条目拖拽(列表→列表);非本应用类型返回空
bool decode_playlist_entries(const QMimeData* mime, PlaylistId* src_pid,
                             QVector<EntryId>* entry_ids)
{
    if (!mime || !src_pid || !entry_ids) {
        return false;
    }
    const QByteArray raw = mime->data(QString::fromLatin1(wusic::kPlaylistEntriesMime));
    if (raw.isEmpty()) {
        return false;
    }
    const QJsonObject root = QJsonDocument::fromJson(raw).object();
    *src_pid               = PlaylistId::from_string(root.value("src").toString());
    if (src_pid->is_null()) {
        return false;
    }
    entry_ids->clear();
    const QJsonArray arr = root.value("ids").toArray();
    for (const auto& v : arr) {
        const EntryId id = EntryId::from_string(v.toString());
        if (!id.is_null()) {
            entry_ids->push_back(id);
        }
    }
    return !entry_ids->isEmpty();
}
} // namespace

PlaylistTreeDropView::PlaylistTreeDropView(QWidget* parent) : QTreeWidget(parent)
{
    setAcceptDrops(true);
}

void PlaylistTreeDropView::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat(QString::fromLatin1(wusic::kLibraryTracksMime)) ||
        event->mimeData()->hasFormat(QString::fromLatin1(wusic::kPlaylistEntriesMime))) {
        event->acceptProposedAction();
        return;
    }
    QTreeWidget::dragEnterEvent(event);
}

void PlaylistTreeDropView::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasFormat(QString::fromLatin1(wusic::kLibraryTracksMime)) ||
        event->mimeData()->hasFormat(QString::fromLatin1(wusic::kPlaylistEntriesMime))) {
        // 允许移动到任意位置;仅落在 Item 上时 dropEvent 才产生动作
        event->acceptProposedAction();
        return;
    }
    QTreeWidget::dragMoveEvent(event);
}

void PlaylistTreeDropView::dropEvent(QDropEvent* event)
{
    // 落到 Item 上才产生动作;背景 → 无操作
    auto* item                  = itemAt(event->position().toPoint());
    auto* playlist_item         = dynamic_cast<WPlayListWidgetItem*>(item);

    // 1) 媒体库曲目拖入
    const QVector<TrackId> tids = decode_library_tracks(event->mimeData());
    if (!tids.isEmpty()) {
        if (playlist_item) {
            emit sgn_library_tracks_dropped(playlist_item->id(), tids);
        }
        event->acceptProposedAction();
        return;
    }
    // 2) 播放列表条目(列表→列表)拖入
    PlaylistId src_pid;
    QVector<EntryId> entry_ids;
    if (decode_playlist_entries(event->mimeData(), &src_pid, &entry_ids)) {
        if (playlist_item) {
            emit sgn_playlist_entries_dropped(src_pid, playlist_item->id(), entry_ids);
        }
        event->acceptProposedAction();
        return;
    }
    // 3) 内部拖动排序
    QTreeWidget::dropEvent(event);
}

PlaylistTreeWidget::PlaylistTreeWidget(QWidget* parent) : QWidget(parent)
{
    init_ui();
    init_connections();
}

void PlaylistTreeWidget::init_ui()
{
    playlist_tree_ = new PlaylistTreeDropView;
    playlist_tree_->setHeaderLabel(tr("Playlist"));
    playlist_tree_->setMinimumWidth(120);
    playlist_tree_->setContextMenuPolicy(Qt::CustomContextMenu);
    playlist_tree_->setRootIsDecorated(false);
    playlist_tree_->setIndentation(0);
    // 选中后单击 → 内联重命名(KDE Dolphin 同款);双击不误触编辑(第二次点击在双击时间窗内触发
    // doubleClicked)
    playlist_tree_->setEditTriggers(QAbstractItemView::SelectedClicked);
    // 内部拖动排序 + 接受外部媒体库拖入(dropEvent 分别处理)
    playlist_tree_->setDragDropMode(QAbstractItemView::InternalMove);
    playlist_tree_->setDefaultDropAction(Qt::MoveAction);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(playlist_tree_);
}

void PlaylistTreeWidget::init_connections()
{
    connect(playlist_tree_, &QTreeWidget::customContextMenuRequested, this,
            &PlaylistTreeWidget::call_tree_context_menu);
    connect(playlist_tree_, &QTreeWidget::itemDoubleClicked, this,
            &PlaylistTreeWidget::on_item_double_clicked);
    connect(playlist_tree_, &QTreeWidget::itemChanged, this, &PlaylistTreeWidget::on_item_changed);
    // 拖动排序:rowsMoved 后收集新顺序提交
    connect(playlist_tree_->model(), &QAbstractItemModel::rowsMoved, this,
            &PlaylistTreeWidget::on_rows_moved);
    // 媒体库曲目拖入某列表项
    connect(playlist_tree_, &PlaylistTreeDropView::sgn_library_tracks_dropped, this,
            &PlaylistTreeWidget::sgn_library_tracks_dropped);
    // 播放列表条目拖入某列表项(列表→列表)
    connect(playlist_tree_, &PlaylistTreeDropView::sgn_playlist_entries_dropped, this,
            &PlaylistTreeWidget::sgn_playlist_entries_dropped);
}

void PlaylistTreeWidget::set_playlists(const QVector<QPair<PlaylistId, QString>>& playlists)
{
    playlist_tree_->blockSignals(true);
    playlist_tree_->clear();
    for (const auto& list : playlists) {
        new WPlayListWidgetItem(playlist_tree_, list.second, list.first);
    }
    playlist_tree_->blockSignals(false);
}

void PlaylistTreeWidget::on_item_double_clicked(QTreeWidgetItem* item)
{
    // 双击切换列表(SelectedClicked 编辑触发器下,双击不会误触内联编辑)
    auto* playlist_item = dynamic_cast<WPlayListWidgetItem*>(item);
    if (playlist_item) {
        emit sgn_switch_playlist(playlist_item->id());
    }
}

void PlaylistTreeWidget::begin_rename(QTreeWidgetItem* item)
{
    if (!item) {
        return;
    }
    playlist_tree_->setCurrentItem(item);
    playlist_tree_->editItem(item, 0);
}

void PlaylistTreeWidget::on_item_changed(QTreeWidgetItem* item)
{
    auto* playlist_item = dynamic_cast<WPlayListWidgetItem*>(item);
    if (!playlist_item) {
        return;
    }
    emit sgn_rename_playlist(playlist_item->id(), item->text(0));
}

void PlaylistTreeWidget::on_rows_moved()
{
    emit sgn_reorder_playlists(collect_order());
}

QVector<PlaylistId> PlaylistTreeWidget::collect_order() const
{
    QVector<PlaylistId> order;
    for (int i = 0; i < playlist_tree_->topLevelItemCount(); ++i) {
        auto* item = dynamic_cast<WPlayListWidgetItem*>(playlist_tree_->topLevelItem(i));
        if (item) {
            order.push_back(item->id());
        }
    }
    return order;
}

void PlaylistTreeWidget::call_tree_context_menu(const QPoint& pos)
{
    QMenu menu(this);
    QAction* act_new      = menu.addAction(tr("New Playlist"));

    QTreeWidgetItem* item = playlist_tree_->itemAt(pos);
    auto* playlist_item   = dynamic_cast<WPlayListWidgetItem*>(item);

    // 背景右键:仅"New Playlist"
    if (!playlist_item) {
        connect(act_new, &QAction::triggered, this, &PlaylistTreeWidget::sgn_create_playlist);
        menu.exec(playlist_tree_->mapToGlobal(pos));
        return;
    }
    const PlaylistId pid = playlist_item->id();

    menu.addSeparator();
    QAction* act_add_track  = menu.addAction(tr("Add track"));
    QAction* act_add_folder = menu.addAction(tr("Add folder"));
    QAction* act_save       = menu.addAction(tr("Save as"));
    QAction* act_rename     = menu.addAction(tr("Rename"));
    QAction* act_copy       = menu.addAction(tr("Copy & Paste"));
    QAction* act_remove     = menu.addAction(tr("Remove"));

    connect(act_new, &QAction::triggered, this, &PlaylistTreeWidget::sgn_create_playlist);
    connect(act_add_track, &QAction::triggered, this,
            [this, pid]() { emit sgn_import_files(pid); });
    connect(act_add_folder, &QAction::triggered, this, [this, pid]() { emit sgn_import_dir(pid); });
    connect(act_save, &QAction::triggered, this, [this, pid]() { emit sgn_save_playlist(pid); });
    connect(act_rename, &QAction::triggered, this, [this, item]() { begin_rename(item); });
    connect(act_copy, &QAction::triggered, this, [this, pid]() { emit sgn_copy_playlist(pid); });
    connect(act_remove, &QAction::triggered, this,
            [this, pid]() { emit sgn_remove_playlist(pid); });

    menu.exec(playlist_tree_->mapToGlobal(pos));
}
