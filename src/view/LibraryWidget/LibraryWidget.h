#pragma once

#include "core/ConfigManager/IConfigurable.h"
#include "core/types.h"

#include <QAbstractItemModel>
#include <QByteArray>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPair>
#include <QPoint>
#include <QSplitter>
#include <QTreeWidget>
#include <QWidget>

class LibraryWidget : public QWidget, public IConfigurable
{
    Q_OBJECT
public:
    explicit LibraryWidget(QAbstractItemModel* song_model, QWidget* parent = nullptr);
    ~LibraryWidget();

    void setSongTreeModel(QAbstractItemModel* model);
    void setPlaylists(const QVector<QPair<PlaylistId, QString>>& playlists);

    QTreeView* songTreeView() const;
    QHeaderView* songTreeHeader() const;

    // config S/L interface
    void loadFromJson(const QJsonObject& json) override;
    QJsonObject saveToJson() override;
    QString configSubKey() const override;

signals:
    void sgnImportFiles(const PlaylistId& pid = PlaylistId());
    void sgnImportDir(const PlaylistId& pid = PlaylistId());

    void sgnPlayTrackByModelIndex(const QModelIndex& index);
    void sgnTrackPropertyRequested(EntryId tid, QString filepath, TrackMetaData meta);
    void sgnRemoveTrackRequested(EntryId tid);
    void sgnRenamePlaylist(PlaylistId id);
    void sgnCopyPlaylist(PlaylistId id);
    void sgnRemovePlaylist(PlaylistId id);
    void sgnSwitchPlaylist(PlaylistId id);

    void sgnPlayTrack(PlaylistId pid, EntryId tid);

    void sgnSavePlaylist(PlaylistId id);

private:
    void initUI();
    void initConnections();

private slots:
    void callTreeContextMenu(const QPoint& pos);
    void callSongContextMenu(const QPoint& pos);
    void updateSongView();

private:
    QTreeView* m_song_tree_view;
    QSplitter* m_main_splitter;
    QTreeWidget* m_playlist_tree;
    QHeaderView* m_song_tree_view_header;

    QHBoxLayout* m_main_layout;
};
