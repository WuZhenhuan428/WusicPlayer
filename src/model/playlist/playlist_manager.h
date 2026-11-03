#pragma once

#include "model/playlist/playlist_context.h"
#include "model/playlist/playlist_repo.h"
#include "model/playlist/playlist_view_model.h"

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
    PlaylistContext* context_ = nullptr;
    PlaylistRepo* repo_       = nullptr;
    PlaylistViewModel* view_  = nullptr;

public:
    explicit PlaylistManager(QObject* parent = nullptr);

public:
    [[nodiscard]] PlaylistViewModel* get_view_model() const;
    [[nodiscard]] QString get_current_track() const;
    [[nodiscard]] QString get_current_playlist_name() const;
    [[nodiscard]] const EntryId& get_current_track_id() const;
    [[nodiscard]] const PlaylistId& get_current_playlist_id() const;
    [[nodiscard]] QVector<PlaylistInfo> get_all_playlists() const;
    [[nodiscard]] QVector<std::shared_ptr<Playlist>> get_playlists() const;
    [[nodiscard]] TrackMetaData get_current_metadata() const;
    [[nodiscard]] QString get_playlist_by_id(const PlaylistId& pid) const;

public slots:
    // receive signals from UI
    void create_playlist() const;
    void remove_playlist(const PlaylistId& to_remove_uuid) const;
    void copy_playlist(const PlaylistId& pid) const;
    // 按给定顺序重排播放列表(拖动排序)
    void reorder_playlists(const QVector<PlaylistId>& ordered_ids) const;
    void load_playlist(const QString& playlist_path) const;
    void rename_playlist(const PlaylistId& src_pid, const QString& dst_name) const;
    void save_playlist(const PlaylistId& pid, const QString& save_path) const;
    void load_cache_after_shown() const;

    // policy 为解析策略;by_operation 时按全局配置 m_add_file_policy + 操作类型默认展开
    void add_track(const PlaylistId& pid, const QString& filepath,
                   AddFilePolicy policy = AddFilePolicy::ByOperation);
    void add_folder(const PlaylistId& pid, const QString& directory,
                    AddFilePolicy policy = AddFilePolicy::ByOperation);
    // 将库曲目(按 TrackId)添加到指定列表;返回成功添加的条目数
    int add_library_tracks(const PlaylistId& pid, const QVector<TrackId>& track_ids);
    // 将 src_pid 列表中的若干条目(按 EntryId)复制到 dst_pid 列表;返回成功添加的条目数
    [[nodiscard]] int copy_tracks_to_playlist(const PlaylistId& src_pid,
                                              const QVector<EntryId>& entry_ids,
                                              const PlaylistId& dst_pid) const;
    void remove_track(const EntryId& tid);
    void remove_missing_tracks();

    // 全局默认解析策略(设置面板配置;持久化由 PlaylistController 负责)
    void set_add_file_policy(AddFilePolicy policy);
    [[nodiscard]] AddFilePolicy add_file_policy() const
    {
        return add_file_policy_;
    }

    // 返回下一/上一曲目的条目身份(EntryId);空表示无曲目可切
    [[nodiscard]] EntryId next_track(PlayMode mode) const;
    [[nodiscard]] EntryId prev_track(PlayMode mode) const;

    void retransmission_playlist_changed();
    void switch_to_playlist(const PlaylistId& pid) const;

    void play(int index);
    // 仅设置当前播放条目(context),不触发播放请求;供"按路径定位"场景使用
    void set_current_track(const EntryId& tid) const;

    // 注入音乐库(非拥有,可空);库变更时自动刷新库引用条目的元数据
    void set_library_manager(LibraryManager* lib);

signals:
    void sgn_request_play(const QString& filepath);
    void sgn_playlist_changed();
    void sgn_cache_load_started();
    void sgn_cache_load_finished(int playlist_count);
    void sgn_playlist_load_started(const PlaylistId& pid, int total_count);
    void sgn_playlist_load_finished(const PlaylistId& pid);

private:
    [[nodiscard]] Track resolve_track(const QString& filepath) const;
    // 把请求策略展开为实际生效策略(by_operation → 全局配置 → 操作类型默认;ask 由上层展开)
    [[nodiscard]] AddFilePolicy resolve_effective_policy(AddFilePolicy requested,
                                                         AddFilePolicy by_operation_default) const;

    LibraryManager* library_       = nullptr;
    AddFilePolicy add_file_policy_ = AddFilePolicy::ByOperation;

private slots:
    void on_library_changed();
};
