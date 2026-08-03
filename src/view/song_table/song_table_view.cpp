#include "view/song_table/song_table_view.h"

#include "model/playlist/playlist_view_model.h"
#include "view/playlist/playlist_widgets.h"

#include <QDesktopServices>
#include <QFileInfo>
#include <QHeaderView>
#include <QJsonObject>
#include <QMenu>
#include <QTreeView>
#include <QUrl>
#include <QVBoxLayout>

SongTableView::SongTableView(QWidget* parent) : QWidget(parent)
{
    init_ui();
    init_connections();
}

void SongTableView::init_ui()
{
    m_tree_view = new QTreeView;
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
    m_tree_view->setRootIsDecorated(false);
    m_tree_view->setIndentation(0);

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
    const EntryId tid = model->track_at(index);
    if (tid.isNull()) {
        return;
    }
    auto* node = static_cast<Node*>(index.internalPointer());
    if (!node) {
        return;
    }
    const TrackMetaData meta = node->meta;
    const QString path       = meta.filepath;

    QMenu menu(this);
    QAction* actPlay          = menu.addAction("&Play");
    QAction* actRemove        = menu.addAction("&Remove");
    QAction* actRemoveMissing = menu.addAction("Remove &Missing Tracks");
    menu.addSeparator();
    QAction* actOpen = menu.addAction("&Open in file explorer");
    QAction* actProp = menu.addAction("Property");

    connect(actPlay, &QAction::triggered, this,
            [this, index]() { emit sgnPlayTrackByModelIndex(index); });

    connect(actProp, &QAction::triggered, this,
            [this, tid, meta, path] { emit sgnTrackPropertyRequested(tid, path, meta); });

    connect(actOpen, &QAction::triggered, this, [path]() {
        QFileInfo file_info(path);
        const QString dir_path = file_info.absolutePath();
        const QUrl url         = QUrl::fromLocalFile(dir_path);
        if (!QDesktopServices::openUrl(url)) {
            qDebug() << "Failed to open folder: " << dir_path;
        }
    });

    connect(actRemove, &QAction::triggered, this,
            [this, tid]() { emit sgnRemoveTrackRequested(tid); });

    connect(actRemoveMissing, &QAction::triggered, this,
            &SongTableView::sgnRemoveMissingTracksRequested);

    menu.exec(m_tree_view->viewport()->mapToGlobal(pos));
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
