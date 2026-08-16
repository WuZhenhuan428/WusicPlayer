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
    for (const QJsonValue& v : arr) {
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
    for (const QJsonValue& v : arr) {
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
            emit sgnLibraryTracksDropped(playlist_item->id(), tids);
        }
        event->acceptProposedAction();
        return;
    }
    // 2) 播放列表条目(列表→列表)拖入
    PlaylistId src_pid;
    QVector<EntryId> entry_ids;
    if (decode_playlist_entries(event->mimeData(), &src_pid, &entry_ids)) {
        if (playlist_item) {
            emit sgnPlaylistEntriesDropped(src_pid, playlist_item->id(), entry_ids);
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
    m_playlist_tree = new PlaylistTreeDropView;
    m_playlist_tree->setHeaderLabel("Playlist");
    m_playlist_tree->setMinimumWidth(120);
    m_playlist_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_playlist_tree->setRootIsDecorated(false);
    m_playlist_tree->setIndentation(0);
    // 选中后单击 → 内联重命名(KDE Dolphin 同款);双击不误触编辑(第二次点击在双击时间窗内触发
    // doubleClicked)
    m_playlist_tree->setEditTriggers(QAbstractItemView::SelectedClicked);
    // 内部拖动排序 + 接受外部媒体库拖入(dropEvent 分别处理)
    m_playlist_tree->setDragDropMode(QAbstractItemView::InternalMove);
    m_playlist_tree->setDefaultDropAction(Qt::MoveAction);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_playlist_tree);
}

void PlaylistTreeWidget::init_connections()
{
    connect(m_playlist_tree, &QTreeWidget::customContextMenuRequested, this,
            &PlaylistTreeWidget::call_tree_context_menu);
    connect(m_playlist_tree, &QTreeWidget::itemDoubleClicked, this,
            &PlaylistTreeWidget::on_item_double_clicked);
    connect(m_playlist_tree, &QTreeWidget::itemChanged, this, &PlaylistTreeWidget::on_item_changed);
    // 拖动排序:rowsMoved 后收集新顺序提交
    connect(m_playlist_tree->model(), &QAbstractItemModel::rowsMoved, this,
            &PlaylistTreeWidget::on_rows_moved);
    // 媒体库曲目拖入某列表项
    connect(m_playlist_tree, &PlaylistTreeDropView::sgnLibraryTracksDropped, this,
            &PlaylistTreeWidget::sgnLibraryTracksDropped);
    // 播放列表条目拖入某列表项(列表→列表)
    connect(m_playlist_tree, &PlaylistTreeDropView::sgnPlaylistEntriesDropped, this,
            &PlaylistTreeWidget::sgnPlaylistEntriesDropped);
}

void PlaylistTreeWidget::set_playlists(const QVector<QPair<PlaylistId, QString>>& playlists)
{
    m_playlist_tree->blockSignals(true);
    m_playlist_tree->clear();
    for (const auto& list : playlists) {
        new WPlayListWidgetItem(m_playlist_tree, list.second, list.first);
    }
    m_playlist_tree->blockSignals(false);
}

void PlaylistTreeWidget::on_item_double_clicked(QTreeWidgetItem* item)
{
    // 双击切换列表(SelectedClicked 编辑触发器下,双击不会误触内联编辑)
    auto* playlist_item = dynamic_cast<WPlayListWidgetItem*>(item);
    if (playlist_item) {
        emit sgnSwitchPlaylist(playlist_item->id());
    }
}

void PlaylistTreeWidget::begin_rename(QTreeWidgetItem* item)
{
    if (!item) {
        return;
    }
    m_playlist_tree->setCurrentItem(item);
    m_playlist_tree->editItem(item, 0);
}

void PlaylistTreeWidget::on_item_changed(QTreeWidgetItem* item)
{
    auto* playlist_item = dynamic_cast<WPlayListWidgetItem*>(item);
    if (!playlist_item) {
        return;
    }
    emit sgnRenamePlaylist(playlist_item->id(), item->text(0));
}

void PlaylistTreeWidget::on_rows_moved()
{
    emit sgnReorderPlaylists(collect_order());
}

QVector<PlaylistId> PlaylistTreeWidget::collect_order() const
{
    QVector<PlaylistId> order;
    for (int i = 0; i < m_playlist_tree->topLevelItemCount(); ++i) {
        auto* item = dynamic_cast<WPlayListWidgetItem*>(m_playlist_tree->topLevelItem(i));
        if (item) {
            order.push_back(item->id());
        }
    }
    return order;
}

void PlaylistTreeWidget::call_tree_context_menu(const QPoint& pos)
{
    QMenu menu(this);
    QAction* actNew       = menu.addAction("New Playlist");

    QTreeWidgetItem* item = m_playlist_tree->itemAt(pos);
    auto* playlist_item   = dynamic_cast<WPlayListWidgetItem*>(item);

    // 背景右键:仅"New Playlist"
    if (!playlist_item) {
        connect(actNew, &QAction::triggered, this, &PlaylistTreeWidget::sgnCreatePlaylist);
        menu.exec(m_playlist_tree->mapToGlobal(pos));
        return;
    }
    const PlaylistId pid = playlist_item->id();

    menu.addSeparator();
    QAction* actAddTrack  = menu.addAction("Add track");
    QAction* actAddFolder = menu.addAction("Add folder");
    QAction* actSave      = menu.addAction("Save as");
    QAction* actRename    = menu.addAction("Rename");
    QAction* actCopy      = menu.addAction("Copy & Paste");
    QAction* actRemove    = menu.addAction("Remove");

    connect(actNew, &QAction::triggered, this, &PlaylistTreeWidget::sgnCreatePlaylist);
    connect(actAddTrack, &QAction::triggered, this, [this, pid]() { emit sgnImportFiles(pid); });
    connect(actAddFolder, &QAction::triggered, this, [this, pid]() { emit sgnImportDir(pid); });
    connect(actSave, &QAction::triggered, this, [this, pid]() { emit sgnSavePlaylist(pid); });
    connect(actRename, &QAction::triggered, this, [this, item]() { begin_rename(item); });
    connect(actCopy, &QAction::triggered, this, [this, pid]() { emit sgnCopyPlaylist(pid); });
    connect(actRemove, &QAction::triggered, this, [this, pid]() { emit sgnRemovePlaylist(pid); });

    menu.exec(m_playlist_tree->mapToGlobal(pos));
}
