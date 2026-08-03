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

    void importFiles(const PlaylistId& pid = PlaylistId());
    void importDir(const PlaylistId& pid = PlaylistId());

    void createNewPlaylist();
    void loadPlaylist();
    void renamePlaylist(const PlaylistId& id = PlaylistId());
    void removePlaylist(const PlaylistId& id = PlaylistId());
    void savePlaylist(const PlaylistId& id = PlaylistId());
    void copyPlaylist(const PlaylistId& id = PlaylistId());
    void removeTrack(const EntryId& id);
    void removeMissingTracks();

    auto viewModel() const -> decltype(std::declval<PlaylistManager*>()->getViewModel());
    QString nextTrack() const;
    QString prevTrack() const;
    void play(int queueIndex);
    void switchToPlaylist(const PlaylistId& id);

    void setPlayMode(PlayMode mode);
    PlayMode playMode() const;

    // 全局默认解析策略(设置面板配置;持久化于本模块 config)
    void setAddFilePolicy(AddFilePolicy policy);
    AddFilePolicy addFilePolicy() const;

    const QVector<std::shared_ptr<Playlist>> playlists() const;
    PlaylistId currentPlaylistId() const;
    EntryId currentTrackId() const;
    const TrackMetaData currentMetadata() const;
    const std::shared_ptr<Playlist> current_playlist();

    std::shared_ptr<Playlist> findPlaylistById(PlaylistId pid);

    void setGroupRules(const QVector<SortRule>& rules);
    void setSortRules(const QVector<SortRule>& rules);
    const QVector<SortRule> groupRules() const;
    const QVector<SortRule> sortRules() const;

    // config S/L interface
    void loadFromJson(const QJsonObject& json) override;
    QJsonObject saveToJson() override;
    QString configSubKey() const override;

    PlaylistId lastPlaylistId() const;
    EntryId lastTrackId() const;

signals:
    void playlistChanged();
    void requestPlay(const QString& path);
    void cacheLoadFinished(int code);
    void playModeChanged(PlayMode mode);

public slots:
    void loadCacheAfterShown();

private:
    // 把全局策略展开为实际生效策略(always_ask 时弹窗询问)
    AddFilePolicy resolvePolicy(AddFilePolicy by_operation_default) const;

private:
    PlaylistManager* m_manager = nullptr;
    QWidget* m_dialogParent    = nullptr;
    PlaylistId m_last_playlist_id;
    EntryId m_last_track_id;
};
