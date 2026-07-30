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
    void setPlaylists(const QVector<QPair<playlistId, QString>>& playlists);

    QTreeView* songTreeView() const;
    QHeaderView* songTreeHeader() const;

    // config S/L interface
    void loadFromJson(const QJsonObject& json) override;
    QJsonObject saveToJson() override;
    QString configSubKey() const override;

signals:
    void sgnImportFiles(const playlistId& pid = playlistId());
    void sgnImportDir(const playlistId& pid = playlistId());

    void sgnPlayTrackByModelIndex(const QModelIndex& index);
    void sgnTrackPropertyRequested(trackId tid, QString filepath, TrackMetaData meta);
    void sgnRemoveTrackRequested(trackId tid);
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
