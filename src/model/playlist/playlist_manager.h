#pragma once

#include "playlist_context.h"
#include "playlist_repo.h"
#include "playlist_view_model.h"

#include <QObject>
#include <QString>
#include <QUuid>
#include <QVector>

class LibraryManager;

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

    // policy 为解析策略;by_operation 时按全局配置 m_add_file_policy + 操作类型默认展开
    void addTrack(const PlaylistId& pid, const QString& filepath,
                  AddFilePolicy policy = AddFilePolicy::by_operation);
    void addFolder(const PlaylistId& pid, const QString& directory,
                   AddFilePolicy policy = AddFilePolicy::by_operation);
    void removeTrack(const EntryId& tid);
    void removeMissingTracks();

    // 全局默认解析策略(设置面板配置;持久化由 PlaylistController 负责)
    void set_add_file_policy(AddFilePolicy policy);
    AddFilePolicy add_file_policy() const
    {
        return m_add_file_policy;
    }

    QString nextTrack(PlayMode mode);
    QString prevTrack(PlayMode mode);

    void retransmissionPlaylistChanged();
    void switchToPlaylist(const PlaylistId& pid);

    void play(int index);

    // 注入音乐库(非拥有,可空);库变更时自动刷新库引用条目的元数据
    void set_library_manager(LibraryManager* lib);

signals:
    void requestPlay(const QString& filepath);
    void playlistChanged();
    void cacheLoadStarted();
    void cacheLoadFinished(int playlistCount);
    void playlistLoadStarted(const PlaylistId& pid, int total_count);
    void playlistLoadFinished(const PlaylistId& pid);

private:
    Track resolve_track(const QString& filepath) const;
    // 把请求策略展开为实际生效策略(by_operation → 全局配置 → 操作类型默认;ask 由上层展开)
    AddFilePolicy resolve_effective_policy(AddFilePolicy requested,
                                           AddFilePolicy by_operation_default) const;

private:
    LibraryManager* m_library       = nullptr;
    AddFilePolicy m_add_file_policy = AddFilePolicy::by_operation;

private slots:
    void on_library_changed();
};
