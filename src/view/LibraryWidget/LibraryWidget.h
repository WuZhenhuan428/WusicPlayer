#pragma once

#include <QWidget>
#include <QSplitter>
#include <QTreeWidget>
#include <QTreeWidget>
#include <QHeaderView>
#include <QLabel>
#include <QAbstractItemModel>
#include <QHBoxLayout>
#include <QPoint>
#include <QByteArray>
#include <QPair>

#include "core/types.h"

class LibraryWidget : public QWidget
{
    Q_OBJECT
public:
    explicit LibraryWidget(QAbstractItemModel* song_model, QWidget *parent = nullptr);
    ~LibraryWidget();

    void setSongTreeModel(QAbstractItemModel* model);
    void setPlaylists(const QVector<QPair<playlistId, QString>>& playlists);

    QByteArray songTreeHeaderState() const;
    void setSongTreeHeaderState(QByteArray state);
    QByteArray splitterState() const;
    void setSplitterState(QByteArray state);
    Qt::Orientation splitterOrientation() const;
    void setSplitterOrientation(Qt::Orientation orient);

    QTreeView* songTreeView() const;
    QHeaderView* songTreeHeader() const;

signals:
    void sgnImportFiles(const playlistId& pid = playlistId());
    void sgnImportDir(const playlistId& pid = playlistId());

    void sgnPlayTrackByModelIndex(const QModelIndex &index);
    void sgnRenamePlaylist(playlistId id);
    void sgnCopyPlaylist(playlistId id);
    void sgnRemovePlaylist(playlistId id);
    void sgnSwitchPlaylist(playlistId id);

    void sgnPlayTrack(playlistId pid, trackId tid);

    void sgnSavePlaylist(playlistId id);

private:
    void initUI();
    void initConnections();

private slots:
    void callTreeContextMenu(const QPoint &pos);
    void updateSongView();

private:
    QTreeView* m_song_tree_view;
    QSplitter* m_main_splitter;
    QTreeWidget* m_playlist_tree;
    QHeaderView* m_song_tree_view_header;

    QHBoxLayout* m_main_layout;
};
