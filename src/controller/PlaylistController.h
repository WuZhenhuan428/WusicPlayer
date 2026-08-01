#pragma once
#include "core/ConfigManager/IConfigurable.h"
#include "core/types.h"
#include "model/playlist/playlist_manager.h"

#include <QByteArray>
#include <QObject>
#include <QVector>

class QJsonObject;

class PlaylistController : public QObject, public IConfigurable
{
    Q_OBJECT
public:
    explicit PlaylistController(PlaylistManager* manager, QWidget* dialog_parent = nullptr,
                                QObject* parent = nullptr);
    ~PlaylistController();

    void importFiles(const playlistId& pid = playlistId());
    void importDir(const playlistId& pid = playlistId());

    void createNewPlaylist();
    void loadPlaylist();
    void renamePlaylist(const playlistId& id = playlistId());
    void removePlaylist(const playlistId& id = playlistId());
    void savePlaylist(const playlistId& id = playlistId());
    void copyPlaylist(const playlistId& id = playlistId());
    void removeTrack(const trackId& id);

    auto viewModel() const -> decltype(std::declval<PlaylistManager*>()->getViewModel());
    QString nextTrack() const;
    QString prevTrack() const;
    void play(int queueIndex);
    void switchToPlaylist(const playlistId& id);

    void setPlayMode(PlayMode mode);
    PlayMode playMode() const;

    const QVector<std::shared_ptr<Playlist>> playlists() const;
    playlistId currentPlaylistId() const;
    trackId currentTrackId() const;
    const TrackMetaData currentMetadata() const;
    const std::shared_ptr<Playlist> current_playlist();

    std::shared_ptr<Playlist> findPlaylistById(playlistId pid);

    void setGroupRules(const QVector<SortRule>& rules);
    void setSortRules(const QVector<SortRule>& rules);
    const QVector<SortRule> groupRules() const;
    const QVector<SortRule> sortRules() const;

    // config S/L interface
    void loadFromJson(const QJsonObject& json) override;
    QJsonObject saveToJson() override;
    QString configSubKey() const override;

    playlistId lastPlaylistId() const;
    trackId lastTrackId() const;

signals:
    void playlistChanged();
    void requestPlay(const QString& path);
    void cacheLoadFinished(int code);
    void playModeChanged(PlayMode mode);

public slots:
    void loadCacheAfterShown();

private:
    PlaylistManager* m_manager = nullptr;
    QWidget* m_dialogParent    = nullptr;
    playlistId m_last_playlist_id;
    trackId m_last_track_id;
};
