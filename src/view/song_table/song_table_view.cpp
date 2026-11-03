#include "view/song_table/song_table_view.h"

#include "model/playlist/playlist_view_model.h"
#include "view/playlist/playlist_widgets.h"

#include <QDesktopServices>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QMimeData>
#include <QTreeView>
#include <QUrl>
#include <QVBoxLayout>

#include <functional>

#include "core/logger/logger_manager.h"
namespace
{
// NOLINTNEXTLINE
Logger* logger = LoggerManager::file_logger("song_table_view", {"console", "gui"});
} // namespace

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
    for (const auto& v : arr) {
        const TrackId tid = TrackId::from_string(v.toString());
        if (!tid.is_null()) {
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
    emit sgn_library_tracks_dropped(tids);
    event->acceptProposedAction();
}

SongTableView::SongTableView(QWidget* parent) : QWidget(parent)
{
    init_ui();
    init_connections();
}

void SongTableView::init_ui()
{
    tree_view_ = new SongTableDropView;
    tree_view_->setContextMenuPolicy(Qt::CustomContextMenu);
    tree_view_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    tree_view_->setSortingEnabled(true);
    tree_view_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tree_view_->setAlternatingRowColors(true);
    tree_header_ = tree_view_->header();
    tree_header_->setSectionResizeMode(0, QHeaderView::Interactive);
    tree_header_->setSectionsMovable(true);
    tree_header_->setFirstSectionMovable(true);
    tree_header_->setMinimumSectionSize(30);
    tree_header_->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    tree_header_->setContextMenuPolicy(Qt::CustomContextMenu);
    tree_view_->setHeaderHidden(false);
    tree_header_->setVisible(true);
    // 组节点(有子节点)显示展开/收起箭头,类似 LibraryBrowser
    tree_view_->setRootIsDecorated(true);
    tree_view_->setIndentation(12);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(tree_view_);
}

void SongTableView::init_connections()
{
    connect(tree_view_, &QTreeView::customContextMenuRequested, this,
            &SongTableView::call_song_context_menu);
    connect(tree_view_, &QTreeView::doubleClicked, this, [this](const QModelIndex& index) {
        if (!index.isValid()) {
            return;
        }
        auto* model = tree_view_->model();
        // 组节点(有子节点)双击 → 展开/折叠, 与 LibraryBrowser 行为一致
        if (model && model->hasChildren(index)) {
            tree_view_->setExpanded(index, !tree_view_->isExpanded(index));
            return;
        }
        emit sgn_play_track_by_model_index(index);
    });
    connect(tree_view_, &SongTableDropView::sgn_library_tracks_dropped, this,
            &SongTableView::sgn_library_tracks_dropped);
    connect(tree_header_, &QHeaderView::customContextMenuRequested, this,
            &SongTableView::show_header_context_menu);
}

void SongTableView::set_model(QAbstractItemModel* model)
{
    tree_view_->setModel(model);
    if (!model) {
        return;
    }
    connect(model, &QAbstractItemModel::modelReset, this, &SongTableView::update_song_view,
            Qt::UniqueConnection);
}

void SongTableView::update_song_view()
{
    QAbstractItemModel* model = tree_view_->model();
    if (!model) {
        return;
    }

    // 批量设置: 分组多时逐次 setExpanded 会触发大量布局/重绘(卡顿来源)
    const bool updates_enabled = tree_view_->updatesEnabled();
    tree_view_->setUpdatesEnabled(false);

    // 递归展开所有分组层级(含多级分组), 组行跨列合并
    std::function<void(const QModelIndex&)> expand_all = [&](const QModelIndex& parent) {
        const int rows = model->rowCount(parent);
        for (int row = 0; row < rows; ++row) {
            const QModelIndex idx = model->index(row, 0, parent);
            if (!model->hasChildren(idx)) {
                continue;
            }
            tree_view_->setFirstColumnSpanned(row, parent, true);
            tree_view_->setExpanded(idx, true);
            expand_all(idx);
        }
    };
    expand_all(QModelIndex());

    tree_view_->setUpdatesEnabled(updates_enabled);
}

QTreeView* SongTableView::tree_view() const
{
    return tree_view_;
}

QHeaderView* SongTableView::tree_header() const
{
    return tree_header_;
}

void SongTableView::call_song_context_menu(const QPoint& pos)
{
    QModelIndex index = tree_view_->indexAt(pos);
    if (!index.isValid()) {
        return;
    }
    index       = index.sibling(index.row(), 0);

    auto* model = qobject_cast<PlaylistViewModel*>(tree_view_->model());
    if (!model) {
        return;
    }

    // 收集选中曲目(过滤组节点:Node::id 为空即组节点)
    QVector<EntryId> selected;
    const auto rows = tree_view_->selectionModel() ? tree_view_->selectionModel()->selectedRows(0)
                                                   : QModelIndexList{};
    if (rows.contains(index)) {
        for (const QModelIndex& idx : rows) {
            auto* node = static_cast<Node*>(idx.internalPointer());
            if (node && !node->id.is_null()) {
                selected.push_back(node->id);
            }
        }
    }
    // 右键不在当前多选集合内 → 按单选处理(只对右键所在行)
    if (selected.isEmpty()) {
        auto* node = static_cast<Node*>(index.internalPointer());
        if (!node || node->id.is_null()) {
            return; // 组节点上右键:不提供曲目操作
        }
        selected.push_back(node->id);
    }

    QMenu menu(this);

    if (selected.size() == 1) {
        // ---- 单选菜单:保持现有功能 + Add to Playlist ----
        auto* node                  = static_cast<Node*>(index.internalPointer());
        const TrackMetaData meta    = node ? node->meta : TrackMetaData{};
        const QString path          = meta.filepath;
        const EntryId tid           = selected.first();

        QAction* act_play           = menu.addAction(tr("&Play"));
        QAction* act_remove         = menu.addAction(tr("&Remove"));
        QAction* act_remove_missing = menu.addAction(tr("Remove &Missing Tracks"));
        menu.addSeparator();
        build_add_to_playlist_menu(&menu, selected);
        menu.addSeparator();
        QAction* act_open = menu.addAction(tr("&Open in file explorer"));
        QAction* act_prop = menu.addAction(tr("Property"));

        connect(act_play, &QAction::triggered, this,
                [this, index]() { emit sgn_play_track_by_model_index(index); });
        connect(act_prop, &QAction::triggered, this,
                [this, tid, meta, path] { emit sgn_track_property_requested(tid, path, meta); });
        connect(act_open, &QAction::triggered, this, [path]() {
            QFileInfo file_info(path);
            const QUrl url = QUrl::fromLocalFile(file_info.absolutePath());
            if (!QDesktopServices::openUrl(url)) {
                logger->debug("Failed to open folder: {}", file_info.absolutePath());
            }
        });
        connect(act_remove, &QAction::triggered, this,
                [this, tid]() { emit sgn_remove_track_requested(tid); });
        connect(act_remove_missing, &QAction::triggered, this,
                &SongTableView::sgn_remove_missing_tracks_requested);
    } else {
        // ---- 多选菜单:仅 Remove(批量)+ Add to Playlist ----
        QAction* act_remove = menu.addAction("&Remove Selected");
        build_add_to_playlist_menu(&menu, selected);
        connect(act_remove, &QAction::triggered, this,
                [this, selected]() { emit sgn_remove_tracks_requested(selected); });
    }

    menu.exec(tree_view_->viewport()->mapToGlobal(pos));
}

void SongTableView::build_add_to_playlist_menu(QMenu* menu, const QVector<EntryId>& track_ids)
{
    if (track_ids.isEmpty() || !playlist_provider_) {
        return;
    }
    auto* sub        = menu->addMenu(tr("Add to Playlist..."));
    const auto lists = playlist_provider_();
    if (lists.isEmpty()) {
        QAction* empty = sub->addAction(tr("(No playlist)"));
        empty->setEnabled(false);
        return;
    }
    for (const auto& [pid, name] : lists) {
        QAction* act = sub->addAction(name);
        connect(act, &QAction::triggered, this,
                [this, pid, track_ids]() { emit sgn_copy_tracks_to_playlist(pid, track_ids); });
    }
}

void SongTableView::set_playlist_list_provider(PlaylistListProvider provider)
{
    playlist_provider_ = std::move(provider);
}

void SongTableView::show_header_context_menu(const QPoint& pos)
{
    const int logical_index = tree_header_->logicalIndexAt(pos);
    QMenu menu(this);
    QAction* act_insert = menu.addAction(tr("Insert Column"));
    QAction* act_rename = menu.addAction(tr("Rename"));
    QAction* act_remove = menu.addAction(tr("Remove"));

    connect(act_insert, &QAction::triggered, this, [this, logical_index]() {
        auto* my_model = qobject_cast<PlaylistViewModel*>(tree_view_->model());
        if (!my_model) {
            return;
        }
        WInsertColumnDialog dialog(tr("Insert Column"), this);
        const int max_index = static_cast<int>(my_model->get_columns().size());
        dialog.set_max_index(max_index);
        dialog.set_index(logical_index);
        if (dialog.exec() == QDialog::Accepted) {
            TableColumn column = dialog.get_rule();
            my_model->insert_column(dialog.index(), column);
        }
    });

    connect(act_rename, &QAction::triggered, this, [this, logical_index]() {
        auto* my_model = qobject_cast<PlaylistViewModel*>(tree_view_->model());
        if (!my_model) {
            return;
        }
        const auto& cols = my_model->get_columns();
        bool ok          = false;
        QString new_name = QInputDialog::getText(
            this, tr("Rename column"), tr("Input new name for this column:"),
            QLineEdit::EchoMode::Normal, cols[logical_index].header_name, &ok);
        if (ok && !new_name.isEmpty()) {
            my_model->rename_column(logical_index, new_name);
        }
    });

    connect(act_remove, &QAction::triggered, this, [this, logical_index]() {
        auto* my_model = qobject_cast<PlaylistViewModel*>(tree_view_->model());
        if (!my_model) {
            return;
        }
        WColumnIndexDialog dialog(tr("Remove column"), tr("The column index will remove"), this);
        const int max_index = static_cast<int>(my_model->get_columns().size() - 1);
        dialog.set_max_index(max_index);
        dialog.set_index(logical_index);
        if (dialog.exec() == QDialog::Accepted) {
            my_model->remove_column(dialog.index());
        }
    });
    menu.exec(tree_header_->mapToGlobal(pos));
}

void SongTableView::load_from_json(const QJsonObject& json)
{
    const QJsonObject obj = json.value(this->config_sub_key()).toObject();
    tree_header_->blockSignals(true);
    tree_header_->restoreState(
        QByteArray::fromBase64(obj.value("tree_header_state").toString().toUtf8()));
    tree_header_->blockSignals(false);
}

QJsonObject SongTableView::save_to_json()
{
    QJsonObject obj;
    obj["tree_header_state"] = QString::fromUtf8(tree_header_->saveState().toBase64());
    return obj;
}

QString SongTableView::config_sub_key() const
{
    return "song_table_view";
}
