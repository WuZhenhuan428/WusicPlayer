#pragma once
#include <QObject>
#include <QVector>
#include <QByteArray>
#include <type_traits>
#include <QWidget>
#include "../../src/LibraryWidget/LibraryWidget.h"
#include "../../src/playlist/playlist_manager.h"
#include "../../src/core/types.h"

class PlaylistController : public QObject
{
    Q_OBJECT
public:
    explicit PlaylistController(PlaylistManager* manager, QWidget* dialog_parent = nullptr, QObject* parent = nullptr);
    ~PlaylistController();

    void importFiles();
    void importDir();

    void createNewPlaylist();
    void loadPlaylist();
    void renamePlaylist(playlistId id = playlistId());
    void removePlaylist(playlistId id = playlistId());
    void savePlaylist(playlistId id = playlistId());
    void copyPlaylist(playlistId id = playlistId());

    auto viewModel() const -> decltype(std::declval<PlaylistManager*>()->getViewModel());
    QString nextTrack(PlayMode mode) const;
    QString prevTrack(PlayMode mode) const;
    void play(int queueIndex);
    void switchToPlaylist(const playlistId& id);

    const QVector<std::shared_ptr<Playlist>> playlists() const;
    playlistId currentPlaylist() const;
    trackId currentTrackId() const;
    TrackMetaData currentMetadata() const;
    PlayMode currentPlayMode() const;

signals:
    void playlistChanged();
    void requestPlay(const QString& path);
    void cacheLoadFinished(int code);

public slots:
    void loadCacheAfterShown();

private:
    PlaylistManager* m_manager = nullptr;
    QWidget* m_dialogParent = nullptr;
};