#include "view/song_table/song_table_view.h"

#include "model/library/library_browse_model.h"
#include "model/playlist/playlist_view_model.h"
#include "view/playlist/playlist_widgets.h"

#include <QDesktopServices>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QHeaderView>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMimeData>
#include <QTreeView>
#include <QUrl>
#include <QVBoxLayout>

namespace
{
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
        const TrackId tid = TrackId::fromString(v.toString());
        if (!tid.isNull()) {
            tids.push_back(tid);
        }
    }
    return tids;
}
} // namespace

SongTableDropView::SongTableDropView(QWidget* parent) : QTreeView(parent)
{
    setAcceptDrops(true);
    // 作为拖拽源:多选曲目 → 拖到播放列表树(列表→列表);不接收自身拖入
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);
}

void SongTableDropView::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasFormat(QString::fromLatin1(wusic::kLibraryTracksMime))) {
        event->acceptProposedAction();
        return;
    }
    QTreeView::dragEnterEvent(event);
}

void SongTableDropView::dragMoveEvent(QDragMoveEvent* event)
{
    if (event->mimeData()->hasFormat(QString::fromLatin1(wusic::kLibraryTracksMime))) {
        event->acceptProposedAction();
        return;
    }
    QTreeView::dragMoveEvent(event);
}

void SongTableDropView::dropEvent(QDropEvent* event)
{
    const QVector<TrackId> tids = decode_library_tracks(event->mimeData());
    if (tids.isEmpty()) {
        QTreeView::dropEvent(event);
        return;
    }
    emit sgnLibraryTracksDropped(tids);
    event->acceptProposedAction();
}

SongTableView::SongTableView(QWidget* parent) : QWidget(parent)
{
    init_ui();
    init_connections();
}

void SongTableView::init_ui()
{
    m_tree_view = new SongTableDropView;
    m_tree_view->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tree_view->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_tree_view->setSortingEnabled(true);
    m_tree_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tree_view->setAlternatingRowColors(true);
    m_tree_header = m_tree_view->header();
    m_tree_header->setSectionResizeMode(0, QHeaderView::Interactive);
    m_tree_header->setSectionsMovable(true);
    m_tree_header->setFirstSectionMovable(true);
    m_tree_header->setMinimumSectionSize(30);
    m_tree_header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_tree_header->setContextMenuPolicy(Qt::CustomContextMenu);
    m_tree_view->setHeaderHidden(false);
    m_tree_header->setVisible(true);
    // 组节点(有子节点)显示展开/收起箭头,类似 LibraryBrowser
    m_tree_view->setRootIsDecorated(true);
    m_tree_view->setIndentation(12);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_tree_view);
}

void SongTableView::init_connections()
{
    connect(m_tree_view, &QTreeView::customContextMenuRequested, this,
            &SongTableView::call_song_context_menu);
    connect(m_tree_view, &QTreeView::doubleClicked, this,
            [this](const QModelIndex& index) { emit sgnPlayTrackByModelIndex(index); });
    connect(m_tree_view, &SongTableDropView::sgnLibraryTracksDropped, this,
            &SongTableView::sgnLibraryTracksDropped);
    connect(m_tree_header, &QHeaderView::customContextMenuRequested, this,
            &SongTableView::show_header_context_menu);
}

void SongTableView::setModel(QAbstractItemModel* model)
{
    m_tree_view->setModel(model);
    if (!model) {
        return;
    }
    connect(model, &QAbstractItemModel::modelReset, this, &SongTableView::update_song_view,
            Qt::UniqueConnection);
}

void SongTableView::update_song_view()
{
    QAbstractItemModel* model = m_tree_view->model();
    if (!model) {
        return;
    }
    for (int i = 0; i < model->rowCount(); ++i) {
        QModelIndex idx = model->index(i, 0);
        if (model->hasChildren(idx)) {
            m_tree_view->setFirstColumnSpanned(i, QModelIndex(), true);
            m_tree_view->setExpanded(idx, true);
        }
    }
}

QTreeView* SongTableView::tree_view() const
{
    return m_tree_view;
}

QHeaderView* SongTableView::tree_header() const
{
    return m_tree_header;
}

void SongTableView::call_song_context_menu(const QPoint& pos)
{
    QModelIndex index = m_tree_view->indexAt(pos);
    if (!index.isValid()) {
        return;
    }
    index       = index.sibling(index.row(), 0);

    auto* model = qobject_cast<PlaylistViewModel*>(m_tree_view->model());
    if (!model) {
        return;
    }

    // 收集选中曲目(过滤组节点:Node::id 为空即组节点)
    QVector<EntryId> selected;
    const auto rows = m_tree_view->selectionModel() ? m_tree_view->selectionModel()->selectedRows(0)
                                                    : QModelIndexList{};
    if (rows.contains(index)) {
        for (const QModelIndex& idx : rows) {
            auto* node = static_cast<Node*>(idx.internalPointer());
            if (node && !node->id.isNull()) {
                selected.push_back(node->id);
            }
        }
    }
    // 右键不在当前多选集合内 → 按单选处理(只对右键所在行)
    if (selected.isEmpty()) {
        auto* node = static_cast<Node*>(index.internalPointer());
        if (!node || node->id.isNull()) {
            return; // 组节点上右键:不提供曲目操作
        }
        selected.push_back(node->id);
    }

    QMenu menu(this);

    if (selected.size() == 1) {
        // ---- 单选菜单:保持现有功能 + Add to Playlist ----
        auto* node                = static_cast<Node*>(index.internalPointer());
        const TrackMetaData meta  = node ? node->meta : TrackMetaData{};
        const QString path        = meta.filepath;
        const EntryId tid         = selected.first();

        QAction* actPlay          = menu.addAction("&Play");
        QAction* actRemove        = menu.addAction("&Remove");
        QAction* actRemoveMissing = menu.addAction("Remove &Missing Tracks");
        menu.addSeparator();
        build_add_to_playlist_menu(&menu, selected);
        menu.addSeparator();
        QAction* actOpen = menu.addAction("&Open in file explorer");
        QAction* actProp = menu.addAction("Property");

        connect(actPlay, &QAction::triggered, this,
                [this, index]() { emit sgnPlayTrackByModelIndex(index); });
        connect(actProp, &QAction::triggered, this,
                [this, tid, meta, path] { emit sgnTrackPropertyRequested(tid, path, meta); });
        connect(actOpen, &QAction::triggered, this, [path]() {
            QFileInfo file_info(path);
            const QUrl url = QUrl::fromLocalFile(file_info.absolutePath());
            if (!QDesktopServices::openUrl(url)) {
                qDebug() << "Failed to open folder: " << file_info.absolutePath();
            }
        });
        connect(actRemove, &QAction::triggered, this,
                [this, tid]() { emit sgnRemoveTrackRequested(tid); });
        connect(actRemoveMissing, &QAction::triggered, this,
                &SongTableView::sgnRemoveMissingTracksRequested);
    } else {
        // ---- 多选菜单:仅 Remove(批量)+ Add to Playlist ----
        QAction* actRemove = menu.addAction("&Remove Selected");
        build_add_to_playlist_menu(&menu, selected);
        connect(actRemove, &QAction::triggered, this,
                [this, selected]() { emit sgnRemoveTracksRequested(selected); });
    }

    menu.exec(m_tree_view->viewport()->mapToGlobal(pos));
}

void SongTableView::build_add_to_playlist_menu(QMenu* menu, const QVector<EntryId>& track_ids)
{
    if (track_ids.isEmpty() || !m_playlist_provider) {
        return;
    }
    auto* sub        = menu->addMenu(tr("Add to Playlist..."));
    const auto lists = m_playlist_provider();
    if (lists.isEmpty()) {
        QAction* empty = sub->addAction(tr("(No playlist)"));
        empty->setEnabled(false);
        return;
    }
    for (const auto& [pid, name] : lists) {
        QAction* act = sub->addAction(name);
        connect(act, &QAction::triggered, this,
                [this, pid, track_ids]() { emit sgnCopyTracksToPlaylist(pid, track_ids); });
    }
}

void SongTableView::set_playlist_list_provider(PlaylistListProvider provider)
{
    m_playlist_provider = std::move(provider);
}

void SongTableView::show_header_context_menu(const QPoint& pos)
{
    const int logical_index = m_tree_header->logicalIndexAt(pos);
    QMenu menu(this);
    QAction* actInsert = menu.addAction("Insert Column Here");
    QAction* actRemove = menu.addAction("Remove This Column");

    connect(actInsert, &QAction::triggered, this, [this, logical_index]() {
        auto* my_model = qobject_cast<PlaylistViewModel*>(m_tree_view->model());
        if (!my_model) {
            return;
        }
        WInsertColumnDialog dialog;
        const int max_index = my_model->get_columns().size();
        dialog.set_max_index(max_index);
        dialog.set_index(logical_index);
        if (dialog.exec() == QDialog::Accepted) {
            TableColumn column = dialog.get_rule();
            my_model->insert_column(dialog.index(), column);
        }
    });
    connect(actRemove, &QAction::triggered, this, [this, logical_index]() {
        auto* my_model = qobject_cast<PlaylistViewModel*>(m_tree_view->model());
        if (!my_model) {
            return;
        }
        WColumnIndexDialog dialog(tr("Remove column"), tr("Input the column index except 0"), this);
        const int max_index = my_model->get_columns().size() - 1;
        dialog.set_max_index(max_index);
        dialog.set_index(logical_index);
        if (dialog.exec() == QDialog::Accepted) {
            my_model->remove_column(dialog.index());
        }
    });
    menu.exec(m_tree_header->mapToGlobal(pos));
}

void SongTableView::load_from_json(const QJsonObject& json)
{
    const QJsonObject obj = json.value(this->config_sub_key()).toObject();
    m_tree_header->blockSignals(true);
    m_tree_header->restoreState(
        QByteArray::fromBase64(obj.value("tree_header_state").toString().toUtf8()));
    m_tree_header->blockSignals(false);
}

QJsonObject SongTableView::save_to_json()
{
    QJsonObject obj;
    obj["tree_header_state"] = QString::fromUtf8(m_tree_header->saveState().toBase64());
    return obj;
}

QString SongTableView::config_sub_key() const
{
    return "song_table_view";
}
