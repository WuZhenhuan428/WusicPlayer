#pragma once

#include <QObject>
#include <QString>
#include <QUuid>
#include <QVector>

#include "playlist_context.h"
#include "playlist_repo.h"
#include "playlist_view_model.h"
#include "../../src/core/utils/AudioUtils.h"

struct PlaylistInfo
{
    playlistId id;
    QString name;
};

class PlaylistManager : public QObject
{
    Q_OBJECT
public:
    PlaylistContext* m_context = nullptr;
    PlaylistRepo* m_repo = nullptr;
    PlaylistViewModel* m_view = nullptr;

public:
    explicit PlaylistManager(QObject *parent = nullptr);
    ~PlaylistManager();

public:
    PlaylistViewModel* getViewModel();
    QString getCurrentTrack() const;
    QString getCurrentPlaylistName() const;
    const trackId& getCurrentTrackId() const;
    const playlistId& getCurrentPlaylist() const;
    QVector<PlaylistInfo> getAllPlaylists();
    QVector<std::shared_ptr<Playlist>> getPlaylists();
    TrackMetaData getCurrentMetadata();
    QString getPlaylistById(const playlistId& pid) const;

public slots:
    // receive signals from UI
    void createPlaylist();
    void removePlaylist(const playlistId& to_remove_uuid);
    void copyPlaylist(const playlistId& pid);
    void loadPlaylist(const QString& playlist_path);
    void renamePlaylist(const playlistId& src_pid, const QString dst_name);
    void savePlaylist(const playlistId& pid, const QString& save_path);
    void loadCacheAfterShown();
    
    void addTrack(const QString& filepath);
    void addFolder(const QString& directory);

    QString nextTrack(PlayMode mode);
    QString prevTrack(PlayMode mode);

    void retransmissionPlaylistChanged();
    void switchToPlaylist(const playlistId& pid);

    void play(int index);

signals:
    void requestPlay(const QString& filepath);
    void playlistChanged();
    void cacheLoadStarted();
    void cacheLoadFinished(int playlistCount);
    void playlistLoadStarted(const playlistId& pid, int total_count);
    void playlistLoadFinished(const playlistId& pid);

private:

};