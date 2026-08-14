#pragma once

#include "core/types.h"
#include <QObject>

class AppContext;
class PlaybackController;
class PlaylistController;

class PlaybackRestoreService : public QObject
{
    Q_OBJECT

public:
    explicit PlaybackRestoreService(AppContext& ctx, QObject* parent);
    ~PlaybackRestoreService();

    void restore();

private:
    AppContext& ctx_;
    PlaylistController* playlist_ctl_ = nullptr;
    PlaybackController* playback_ctl_ = nullptr;

private:
    int find_queue_index_by_track_id(const EntryId& tid);
    void finalize_restore_when_ready(int retry);
    void on_cache_load_finished();
    void on_model_reset();

    PlaylistId pending_pid_;
    EntryId pending_tid_;
    int pending_pos_ms_         = 0;
    bool pending_should_resume_ = false;

    bool restored_              = false;
};
