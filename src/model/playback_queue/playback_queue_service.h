#pragma once

#include "core/types.h"
#include "model/playback_queue/playback_queue.h"

#include <QObject>
#include <QString>

class LibraryManager;
class PlaylistManager;

/**
 * @brief 现在播放队列门面:来源构建(播放列表 / 媒体库 / 外部文件)、
 * 队列持久化、信号转发。
 *
 * 依赖注入的 PlaylistManager / LibraryManager 均为非拥有指针,可空;
 * 为空时对应来源构建返回失败。队列对象归本服务所有。
 *
 * 本阶段为积累期:只提供模块与信号,不接入现有播放路径;
 * 切断点由上层消费队列的当前项变化驱动播放。
 */
class PlaybackQueueService : public QObject
{
    Q_OBJECT
public:
    explicit PlaybackQueueService(QObject* parent = nullptr);

    // 依赖注入(非拥有,可空)
    void set_playlist_manager(PlaylistManager* mgr);
    void set_library_manager(LibraryManager* lib);

    // 非拥有:队列归本服务所有
    PlaybackQueue* queue();

    // ---- 来源构建 ----
    // 播放列表条目:经 PlaylistManager 解析出 filepath + meta 快照
    bool enqueue_playlist_entry(const PlaylistId& pid, const EntryId& eid);
    // 媒体库曲目:经 LibraryManager 解析
    bool enqueue_library_track(const TrackId& track_id);
    // 外部文件:路径规范化后入队;返回新项下标
    int enqueue_external(const QString& filepath, const TrackMetaData& meta = TrackMetaData{});

    // ---- 播放(入队 + 设为当前 + 发 sgn_play_requested) ----
    // 媒体库曲目;失败(未注入 / 曲目不存在)返回 false
    bool play_library_track(const TrackId& track_id);
    // 外部文件;返回新项下标
    int play_external(const QString& filepath, const TrackMetaData& meta = TrackMetaData{});

    // ---- 持久化 ----
    bool save_to(const QString& path) const;
    bool load_from(const QString& path);

signals:
    void sgn_queue_changed();
    void sgn_current_changed(int index);
    // 当前项变化并请求播放(积累期由上层接 filepath 播放;切断点接 PlaybackService)
    void sgn_play_requested(const QueueItem& item);

private:
    PlaybackQueue m_queue;
    PlaylistManager* m_playlist_mgr = nullptr;
    LibraryManager* m_library       = nullptr;
};
