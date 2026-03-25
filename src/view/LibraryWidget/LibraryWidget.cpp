#include "LibraryWidget.h"
#include "model/playlist/playlist_view_model.h"
#include "view/playlist/playlist_widgets.h"
#include <QMenu>

LibraryWidget::LibraryWidget(QAbstractItemModel* song_model, QWidget *parent)
    : QWidget(parent)
{
    this->initUI();
    this->initConnections();
    this->setSongTreeModel(song_model);
}

LibraryWidget::~LibraryWidget() {}

void LibraryWidget::initConnections() {
    connect(m_playlist_tree, &QTreeWidget::customContextMenuRequested, this, &LibraryWidget::callTreeContextMenu);

    connect(m_song_tree_view, &QTreeView::doubleClicked, this, [this](const QModelIndex &index) {
        emit sgnPlayTrackByModelIndex(index);
    });

    connect(m_playlist_tree, &QTreeWidget::itemDoubleClicked, this,
        [this](QTreeWidgetItem *item){
            WPlayListWidgetItem* temp = dynamic_cast<WPlayListWidgetItem*>(item);
            if(temp) {
                emit sgnSwitchPlaylist(temp->id());
            }
        }
    );

    connect(m_song_tree_view_header, &QHeaderView::customContextMenuRequested, this, [this](const QPoint& pos){
        int logical_index = m_song_tree_view_header->logicalIndexAt(pos);
        QMenu menu(this);
        QAction* actInsert = menu.addAction("Insert Column Here");
        QAction* actRemove = menu.addAction("Remove This Column");

        connect(actInsert, &QAction::triggered, [this, logical_index](){
            PlaylistViewModel* my_model = dynamic_cast<PlaylistViewModel*>(m_song_tree_view->model());
            if (!my_model) return;
            WInsertColumnDialog dialog;
            int maxIndex = my_model->getColumns().size();
            dialog.setMaxIndex(maxIndex);
            dialog.setIndex(logical_index);
            if (dialog.exec() == QDialog::Accepted) {
                TableColumn column = dialog.getRule();
                my_model->insertColumn(dialog.index(), column);
            }
        });
        connect(actRemove, &QAction::triggered, [this, logical_index](){
            PlaylistViewModel* my_model = dynamic_cast<PlaylistViewModel*>(m_song_tree_view->model());
            WColumnIndexDialog dialog(tr("Remove column"), tr("Input the column index except 0"), this);
            int maxIndex = my_model->getColumns().size() - 1;
            dialog.setMaxIndex(maxIndex);
            dialog.setIndex(logical_index);
            if (dialog.exec() == QDialog::Accepted) {
                my_model->removeColumn(dialog.index());
            }
        });
        menu.exec(m_song_tree_view_header->mapToGlobal(pos));
    });
}

#include "../playlist/playlist_widgets.h"
#include <QMenu>
#include <QInputDialog>

// old: onTreeContextMenuRequested
void LibraryWidget::callTreeContextMenu(const QPoint &pos) {
    QTreeWidgetItem* item = m_playlist_tree->itemAt(pos);
    if (!item) return;

    WPlayListWidgetItem* playlist_item = dynamic_cast<WPlayListWidgetItem*>(item);
    if (!playlist_item) return;

    playlistId pid = playlist_item->id();
    QMenu menu(this);
    QAction* actAddTrack = menu.addAction("Add track");
    QAction* actAddFolder = menu.addAction("Add folder");
    QAction* actSave = menu.addAction("Save as");
    QAction* actRename = menu.addAction("Rename");
    QAction* actCopy = menu.addAction("Copy");
    QAction* actRemove = menu.addAction("Remove");

    connect(actAddTrack, &QAction::triggered, this, [this, pid](){ emit sgnImportFiles(pid); });
    connect(actAddFolder, &QAction::triggered, this, [this, pid](){ emit sgnImportDir(pid); });
    connect(actSave, &QAction::triggered, this, [this, pid](){ emit sgnSavePlaylist(pid); });

    connect(actRename, &QAction::triggered, this, [this, pid](){ emit sgnRenamePlaylist(pid); });

    connect(actCopy, &QAction::triggered, this, [this, pid](){ emit sgnCopyPlaylist(pid); });

    connect(actRemove, &QAction::triggered, this, [this, pid](){ emit sgnRemovePlaylist(pid); });

    menu.exec(m_playlist_tree->mapToGlobal(pos));
}

void LibraryWidget::initUI() {
    m_playlist_tree = new QTreeWidget;
    m_playlist_tree->setHeaderLabel("Playlist");
    m_playlist_tree->setMinimumWidth(120);
    m_playlist_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_playlist_tree->setRootIsDecorated(false);
    m_playlist_tree->setIndentation(0);

    m_song_tree_view = new QTreeView;
    m_song_tree_view->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_song_tree_view->setSortingEnabled(true);
    m_song_tree_view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_song_tree_view->setAlternatingRowColors(true);
    m_song_tree_view_header = m_song_tree_view->header();
    m_song_tree_view_header->setSectionResizeMode(0, QHeaderView::Interactive);
    m_song_tree_view_header->setSectionsMovable(true);
    m_song_tree_view_header->setFirstSectionMovable(true);
    m_song_tree_view_header->setMinimumSectionSize(30);
    m_song_tree_view_header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_song_tree_view_header->setContextMenuPolicy(Qt::CustomContextMenu);
    m_song_tree_view->setHeaderHidden(false);
    m_song_tree_view_header->setVisible(true);
    m_song_tree_view->setRootIsDecorated(false);
    m_song_tree_view->setIndentation(0);

    m_main_splitter = new QSplitter(Qt::Horizontal, this);
    m_main_splitter->addWidget(m_playlist_tree);
    m_main_splitter->addWidget(m_song_tree_view);
    m_main_splitter->setStretchFactor(0, 1);
    m_main_splitter->setStretchFactor(1, 3);
    m_main_splitter->setChildrenCollapsible(false);

    m_main_layout = new QHBoxLayout;
    m_main_layout->addWidget(m_main_splitter);

    this->setLayout(m_main_layout);
}

void LibraryWidget::setSongTreeModel(QAbstractItemModel* model) {
    m_song_tree_view->setModel(model);
    if (!model) return;
    connect(model, &QAbstractItemModel::modelReset, this, &LibraryWidget::updateSongView, Qt::UniqueConnection);
}

void LibraryWidget::setPlaylists(const QVector<QPair<playlistId, QString>>& playlists) {
    this->m_playlist_tree->clear();
    for (const auto& list : playlists) {
        new WPlayListWidgetItem(this->m_playlist_tree, list.second, list.first);
    }
}

QByteArray LibraryWidget::songTreeHeaderState() const {
    return this->m_song_tree_view_header->saveState();
}

void LibraryWidget::setSongTreeHeaderState(QByteArray state) {
    // Block signals to prevent sortIndicatorChanged from triggering
    // PlaylistViewModel::sort(), which would override the configured sort rules
    m_song_tree_view_header->blockSignals(true);
    m_song_tree_view_header->restoreState(state);
    m_song_tree_view_header->blockSignals(false);
}

QByteArray LibraryWidget::splitterState() const {
    return this->m_main_splitter->saveState();
}

void LibraryWidget::setSplitterState(QByteArray state) {
    this->m_main_splitter->restoreState(state);
}

Qt::Orientation LibraryWidget::splitterOrientation() const {
    return this->m_main_splitter->orientation();
}

void LibraryWidget::setSplitterOrientation(Qt::Orientation orient) {
    this->m_main_splitter->setOrientation(orient);
}


void LibraryWidget::updateSongView() {
    QAbstractItemModel* model = m_song_tree_view->model();
    if (!model) return;
    for (int i = 0; i < model->rowCount(); ++i) {
        QModelIndex idx = model->index(i, 0);
        if (model->hasChildren(idx)) {
            m_song_tree_view->setFirstColumnSpanned(i, QModelIndex(), true);
            m_song_tree_view->setExpanded(idx, true);
        }
    }
}

QTreeView* LibraryWidget::songTreeView() const {
    return m_song_tree_view;
}

QHeaderView* LibraryWidget::songTreeHeader() const {
    return m_song_tree_view_header;
}