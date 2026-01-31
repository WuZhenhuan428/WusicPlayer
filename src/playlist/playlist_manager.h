#pragma once

#include <QObject>
#include <QString>
#include <QUuid>
#include <QVector>

#include "playlist_context.h"
#include "playlist_repo.h"
#include "playlist_view_model.h"
#include "../../include/audio.h"

struct PlaylistInfo
{
    QUuid id;
    QString name;
};

class PlaylistManager : public QObject
{
    Q_OBJECT

    PlaylistContext* m_context = new PlaylistContext;
    PlaylistRepo* m_repo = new PlaylistRepo;
    PlaylistViewModel* m_view = new PlaylistViewModel(m_repo);

public:
    explicit PlaylistManager(QObject *parent = nullptr);
    ~PlaylistManager();

public:
    PlaylistViewModel* getViewModel();
    const QUuid& getCurrentPlaylist() const;
    QVector<PlaylistInfo> getAllPlaylists();
    QVector<std::shared_ptr<Playlist>> getPlaylists();

public slots:
    // receive signals from UI
    void createPlaylist();
    void removePlaylist(const QUuid& to_remove_uuid);
    void copyPlaylist(const QUuid& playlist_id);
    void loadPlaylist(const QString& playlist_path);
    void renamePlaylist(const QUuid& src_uuid, const QString dst_name);
    void saveCurrentPlaylist(const QString& save_path);

    void addTrack(const QString& filepath);
    void addFolder(const QString& directory);

    QString nextTrack();
    QString prevTrack();

    void retransmissionPlaylistChanged();
    void switchToPlaylist(const QUuid& id);

    void play(int index);

signals:
    void requestPlay(const QString& filepath);
    void playlistChanged();

private:

};