#pragma once

#include "playlist_context.h"
#include "playlist_repo.h"
#include "playlist_view_model.h"

#include <QObject>
#include <QString>
#include <QUuid>
#include <QVector>

struct PlaylistInfo
{
    PlaylistId id;
    QString name;
};

class PlaylistManager : public QObject
{
    Q_OBJECT
public:
    PlaylistContext* m_context = nullptr;
    PlaylistRepo* m_repo       = nullptr;
    PlaylistViewModel* m_view  = nullptr;

public:
    explicit PlaylistManager(QObject* parent = nullptr);
    ~PlaylistManager();

public:
    PlaylistViewModel* getViewModel();
    QString getCurrentTrack() const;
    QString getCurrentPlaylistName() const;
    const EntryId& getCurrentTrackId() const;
    const PlaylistId& getCurrentPlaylistId() const;
    QVector<PlaylistInfo> getAllPlaylists();
    QVector<std::shared_ptr<Playlist>> getPlaylists();
    TrackMetaData getCurrentMetadata();
    QString getPlaylistById(const PlaylistId& pid) const;

public slots:
    // receive signals from UI
    void createPlaylist();
    void removePlaylist(const PlaylistId& to_remove_uuid);
    void copyPlaylist(const PlaylistId& pid);
    void loadPlaylist(const QString& playlist_path);
    void renamePlaylist(const PlaylistId& src_pid, const QString dst_name);
    void savePlaylist(const PlaylistId& pid, const QString& save_path);
    void loadCacheAfterShown();

    void addTrack(const PlaylistId& pid, const QString& filepath);
    void addFolder(const PlaylistId& pid, const QString& directory);
    void removeTrack(const EntryId& tid);

    QString nextTrack(PlayMode mode);
    QString prevTrack(PlayMode mode);

    void retransmissionPlaylistChanged();
    void switchToPlaylist(const PlaylistId& pid);

    void play(int index);

signals:
    void requestPlay(const QString& filepath);
    void playlistChanged();
    void cacheLoadStarted();
    void cacheLoadFinished(int playlistCount);
    void playlistLoadStarted(const PlaylistId& pid, int total_count);
    void playlistLoadFinished(const PlaylistId& pid);

private:
};
