#pragma once
#include "core/config_manager/i_configurable.h"
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

    void import_files(const PlaylistId& pid = PlaylistId());
    void import_dir(const PlaylistId& pid = PlaylistId());

    void create_new_playlist();
    void load_playlist();
    // 弹窗重命名(保留)
    void rename_playlist(const PlaylistId& id = PlaylistId());
    // 直接改名(内联编辑提交)
    void rename_playlist(const PlaylistId& id, const QString& new_name);
    void remove_playlist(const PlaylistId& id = PlaylistId());
    void save_playlist(const PlaylistId& id = PlaylistId());
    void copy_playlist(const PlaylistId& id = PlaylistId());
    // 按给定顺序重排播放列表(拖动排序)
    void reorder_playlists(const QVector<PlaylistId>& ordered_ids);
    void remove_track(const EntryId& id);
    void remove_tracks(const QVector<EntryId>& ids);
    void remove_missing_tracks();
    // 将库曲目(按 TrackId)添加到指定列表;返回成功添加的条目数
    int add_library_tracks(const PlaylistId& pid, const QVector<TrackId>& track_ids);
    // 将 src_pid 列表中的若干条目复制到 dst_pid 列表;返回成功添加的条目数
    int copy_tracks_to_playlist(const PlaylistId& src_pid, const QVector<EntryId>& entry_ids,
                                const PlaylistId& dst_pid);

    auto view_model() const -> decltype(std::declval<PlaylistManager*>()->get_view_model());
    // 返回下一/上一曲目的条目身份(EntryId);空表示无曲目可切
    EntryId next_track() const;
    EntryId prev_track() const;
    // 条目身份 → 播放路径(当前播放列表;找不到返回空)
    QString track_file_path(const EntryId& eid) const;
    void play(int queueIndex);
    // 在当前播放列表中按路径定位并设置 context(play_track);找到返回 true(供定位同步),
    // 找不到(库直播/外部/不在列表)返回 false。不触发播放,由调用方统一播放。
    bool locate_filepath(const QString& filepath);
    void switch_to_playlist(const PlaylistId& id);

    void set_play_mode(PlayMode mode);
    PlayMode play_mode() const;

    // 全局默认解析策略(设置面板配置;持久化于本模块 config)
    void set_add_file_policy(AddFilePolicy policy);
    AddFilePolicy add_file_policy() const;

    const QVector<std::shared_ptr<Playlist>> playlists() const;
    PlaylistId current_playlist_id() const;
    EntryId current_track_id() const;
    const TrackMetaData current_metadata() const;
    const std::shared_ptr<Playlist> current_playlist();

    std::shared_ptr<Playlist> find_playlist_by_id(PlaylistId pid);

    void set_group_rules(const QVector<SortRule>& rules);
    void set_sort_rules(const QVector<SortRule>& rules);
    const QVector<SortRule> group_rules() const;
    const QVector<SortRule> sort_rules() const;

    // config S/L interface
    void load_from_json(const QJsonObject& json) override;
    QJsonObject save_to_json() override;
    QString config_sub_key() const override;

    PlaylistId last_playlist_id() const;
    EntryId last_track_id() const;

signals:
    void sgn_playlist_changed();
    void sgn_request_play(const QString& path);
    void sgn_cache_load_finished(int code);
    void sgn_play_mode_changed(PlayMode mode);

public slots:
    void load_cache_after_shown();

private:
    // 把全局策略展开为实际生效策略(always_ask 时弹窗询问)
    AddFilePolicy resolve_policy(AddFilePolicy by_operation_default) const;

private:
    PlaylistManager* m_manager = nullptr;
    QWidget* m_dialogParent    = nullptr;
    PlaylistId m_last_playlist_id;
    EntryId m_last_track_id;
};
