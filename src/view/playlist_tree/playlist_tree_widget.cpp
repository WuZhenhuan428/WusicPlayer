#include "view/playlist_tree/playlist_tree_widget.h"

#include "view/playlist/playlist_widgets.h"

#include <QMenu>
#include <QTreeWidget>
#include <QVBoxLayout>

PlaylistTreeWidget::PlaylistTreeWidget(QWidget* parent) : QWidget(parent)
{
    init_ui();
    init_connections();
}

void PlaylistTreeWidget::init_ui()
{
    m_playlist_tree = new QTreeWidget;
    m_playlist_tree->setHeaderLabel("Playlist");
    m_playlist_tree->setMinimumWidth(120);
    m_playlist_tree->setContextMenuPolicy(Qt::CustomContextMenu);
    m_playlist_tree->setRootIsDecorated(false);
    m_playlist_tree->setIndentation(0);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_playlist_tree);
}

void PlaylistTreeWidget::init_connections()
{
    connect(m_playlist_tree, &QTreeWidget::customContextMenuRequested, this,
            &PlaylistTreeWidget::call_tree_context_menu);
    connect(m_playlist_tree, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem* item) {
        auto* playlist_item = dynamic_cast<WPlayListWidgetItem*>(item);
        if (playlist_item) {
            emit sgnSwitchPlaylist(playlist_item->id());
        }
    });
}

void PlaylistTreeWidget::set_playlists(const QVector<QPair<PlaylistId, QString>>& playlists)
{
    m_playlist_tree->clear();
    for (const auto& list : playlists) {
        new WPlayListWidgetItem(m_playlist_tree, list.second, list.first);
    }
}

void PlaylistTreeWidget::call_tree_context_menu(const QPoint& pos)
{
    QTreeWidgetItem* item = m_playlist_tree->itemAt(pos);
    if (!item) {
        return;
    }
    auto* playlist_item = dynamic_cast<WPlayListWidgetItem*>(item);
    if (!playlist_item) {
        return;
    }
    const PlaylistId pid = playlist_item->id();

    QMenu menu(this);
    QAction* actAddTrack  = menu.addAction("Add track");
    QAction* actAddFolder = menu.addAction("Add folder");
    QAction* actSave      = menu.addAction("Save as");
    QAction* actRename    = menu.addAction("Rename");
    QAction* actCopy      = menu.addAction("Copy");
    QAction* actRemove    = menu.addAction("Remove");

    connect(actAddTrack, &QAction::triggered, this, [this, pid]() { emit sgnImportFiles(pid); });
    connect(actAddFolder, &QAction::triggered, this, [this, pid]() { emit sgnImportDir(pid); });
    connect(actSave, &QAction::triggered, this, [this, pid]() { emit sgnSavePlaylist(pid); });
    connect(actRename, &QAction::triggered, this, [this, pid]() { emit sgnRenamePlaylist(pid); });
    connect(actCopy, &QAction::triggered, this, [this, pid]() { emit sgnCopyPlaylist(pid); });
    connect(actRemove, &QAction::triggered, this, [this, pid]() { emit sgnRemovePlaylist(pid); });

    menu.exec(m_playlist_tree->mapToGlobal(pos));
}
