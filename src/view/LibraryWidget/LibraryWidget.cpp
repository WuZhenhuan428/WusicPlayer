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
    connect(m_playlistTree, &QTreeWidget::customContextMenuRequested, this, &LibraryWidget::callTreeContextMenu);

    connect(m_songTreeView, &QTreeView::doubleClicked, this, [this](const QModelIndex &index) {
        emit sgnPlayTrackByModelIndex(index);
    });

    connect(m_playlistTree, &QTreeWidget::itemDoubleClicked, this,
        [this](QTreeWidgetItem *item, int column){
            WPlayListWidgetItem* temp = dynamic_cast<WPlayListWidgetItem*>(item);
            if(temp) {
                emit sgnSwitchPlaylist(temp->id());
            }
        }
    );

    connect(m_songTreeViewHeader, &QHeaderView::customContextMenuRequested, this, [this](const QPoint& pos){
        int logical_index = m_songTreeViewHeader->logicalIndexAt(pos);
        QMenu menu(this);
        QAction* actInsert = menu.addAction("Insert Column Here");
        QAction* actRemove = menu.addAction("Remove This Column");

        connect(actInsert, &QAction::triggered, [this, logical_index](){
            auto* my_model = dynamic_cast<PlaylistViewModel*>(m_songTreeView->model());
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
            auto* my_model = dynamic_cast<PlaylistViewModel*>(m_songTreeView->model());
            WColumnIndexDialog dialog(tr("Remove column"), tr("Input the column index except 0"), this);
            int maxIndex = my_model->getColumns().size() - 1;
            dialog.setMaxIndex(maxIndex);
            dialog.setIndex(logical_index);
            if (dialog.exec() == QDialog::Accepted) {
                my_model->removeColumn(dialog.index());
            }
        });
        menu.exec(m_songTreeViewHeader->mapToGlobal(pos));
    });
}

#include "../playlist/playlist_widgets.h"
#include <QMenu>
#include <QInputDialog>

// old: onTreeContextMenuRequested
void LibraryWidget::callTreeContextMenu(const QPoint &pos) {
    QTreeWidgetItem* item = m_playlistTree->itemAt(pos);
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

    connect(actAddTrack, &QAction::triggered, this, [this](){ emit sgnImportFiles(); });   // to controller
    connect(actAddFolder, &QAction::triggered, this, [this](){ emit sgnImportDir(); });
    connect(actSave, &QAction::triggered, this, [this, pid](){ emit sgnSavePlaylist(pid); });

    connect(actRename, &QAction::triggered, this, [this, pid](){ emit sgnRenamePlaylist(pid); });

    connect(actCopy, &QAction::triggered, this, [this, pid](){ emit sgnCopyPlaylist(pid); });

    connect(actRemove, &QAction::triggered, this, [this, pid](){ emit sgnRemovePlaylist(pid); });

    menu.exec(m_playlistTree->mapToGlobal(pos));
}

void LibraryWidget::initUI() {
    m_playlistTree = new QTreeWidget;
    m_playlistTree->setHeaderLabel("Playlist");
    m_playlistTree->setMinimumWidth(120);
    m_playlistTree->setContextMenuPolicy(Qt::CustomContextMenu);

    m_songTreeView = new QTreeView;
    m_songTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_songTreeView->setSortingEnabled(true);
    m_songTreeView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_songTreeView->setAlternatingRowColors(true);
    m_songTreeViewHeader = m_songTreeView->header();
    m_songTreeViewHeader->setSectionResizeMode(0, QHeaderView::Interactive);
    m_songTreeViewHeader->setSectionsMovable(true);
    m_songTreeViewHeader->setFirstSectionMovable(true);
    m_songTreeViewHeader->setMinimumSectionSize(30);
    m_songTreeViewHeader->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_songTreeViewHeader->setContextMenuPolicy(Qt::CustomContextMenu);
    m_songTreeView->setHeaderHidden(false);
    m_songTreeViewHeader->setVisible(true);

    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter->addWidget(m_playlistTree);
    m_mainSplitter->addWidget(m_songTreeView);
    m_mainSplitter->setStretchFactor(0, 1);
    m_mainSplitter->setStretchFactor(1, 3);
    m_mainSplitter->setChildrenCollapsible(false);

    m_mainLayout = new QHBoxLayout;
    m_mainLayout->addWidget(m_mainSplitter);

    this->setLayout(m_mainLayout);
}

void LibraryWidget::setSongTreeModel(QAbstractItemModel* model) {
    m_songTreeView->setModel(model);
    if (!model) return;
    connect(model, &QAbstractItemModel::modelReset, this, &LibraryWidget::updateSongView, Qt::UniqueConnection);
}

void LibraryWidget::setPlaylists(const QVector<QPair<playlistId, QString>>& playlists) {
    this->m_playlistTree->clear();
    for (const auto& list : playlists) {
        new WPlayListWidgetItem(this->m_playlistTree, list.second, list.first);
    }
}

LibraryWidgetStates LibraryWidget::getStates() const {
    LibraryWidgetStates states;
    states.splitterState = m_mainSplitter->saveState();
    states.songTreeViewHeaderState = m_songTreeViewHeader->saveState();
    return states;
}

void LibraryWidget::setStates(LibraryWidgetStates states) {
    m_songTreeViewHeader->restoreState(states.songTreeViewHeaderState);
    m_mainSplitter->restoreState(states.splitterState);
}


void LibraryWidget::updateSongView() {
    QAbstractItemModel* model = m_songTreeView->model();
    if (!model) return;
    for (int i = 0; i < model->rowCount(); ++i) {
        QModelIndex idx = model->index(i, 0);
        if (model->hasChildren(idx)) {
            m_songTreeView->setFirstColumnSpanned(i, QModelIndex(), true);
            m_songTreeView->setExpanded(idx, true);
        }
    }
}

QTreeView* LibraryWidget::songTreeView() const {
    return m_songTreeView;
}

QHeaderView* LibraryWidget::songTreeHeader() const {
    return m_songTreeViewHeader;
}