#include "service/playback_service.h"

#include "core/utils/audio.hpp"

#include "app_context.h"
#include "controller/playback_controller.h"
#include "controller/playlist_controller.h"
#include "view/control_bar/control_bar.h"
#include "view/main_window.h"

PlaybackService::PlaybackService(AppContext& ctx, QObject* parent) : QObject(parent), ctx_(ctx)
{
    assert(ctx_.main_window && ctx_.playback_controller && ctx.playlist_controller);
    assert(ctx_.main_window->control_bar_widget());

    this->main_window_  = this->ctx_.main_window;
    this->control_bar_  = this->ctx_.main_window->control_bar_widget();
    this->playback_ctl_ = this->ctx_.playback_controller;
    this->playlist_ctl_ = this->ctx_.playlist_controller;
}

void PlaybackService::bind()
{
    if (bound_) {
        return;
    }
    connect(ctx_.main_window, &MainWindow::sgn_play_track_requested, this,
            &PlaybackService::handle_play_track_request);
    connect(control_bar_, &ControlBar::sgn_btn_play_pause_clicked, playback_ctl_,
            [this](bool is_request_play) {
                if (is_request_play) {
                    playback_ctl_->play();
                } else {
                    playback_ctl_->pause();
                }
            });
    connect(control_bar_, &ControlBar::sgn_btn_stop_clicked, playback_ctl_,
            &PlaybackController::stop);
    connect(control_bar_, &ControlBar::sgn_btn_mute_clicked, playback_ctl_,
            &PlaybackController::flip_mute);
    connect(control_bar_, &ControlBar::sgn_in_order, this,
            [this]() { playlist_ctl_->set_play_mode(PlayMode::InOrder); });
    connect(control_bar_, &ControlBar::sgn_loop, this,
            [this]() { playlist_ctl_->set_play_mode(PlayMode::Loop); });
    connect(control_bar_, &ControlBar::sgn_shuffle, this,
            [this]() { playlist_ctl_->set_play_mode(PlayMode::Shuffle); });
    connect(control_bar_, &ControlBar::sgn_out_of_order_by_track, this,
            [this]() { playlist_ctl_->set_play_mode(PlayMode::OutOfOrderByTrack); });
    connect(control_bar_, &ControlBar::sgn_out_of_order_by_group, this,
            [this]() { playlist_ctl_->set_play_mode(PlayMode::OutOfOrderByGroup); });
    connect(control_bar_, &ControlBar::sgn_slider_position_released, this, [this](int percent) {
        playback_ctl_->set_position(static_cast<qint64>(percent) * 1000);
    });
    connect(control_bar_, &ControlBar::sgn_slider_volume_released, playback_ctl_,
            &PlaybackController::set_volume);
    connect(control_bar_, &ControlBar::sgn_slider_volume_moved, playback_ctl_,
            &PlaybackController::set_volume);
    connect(control_bar_, &ControlBar::sgn_select_device_id, playback_ctl_,
            &PlaybackController::set_device_by_id);
    // 跟随系统设备: UI → 控制器; 控制器状态 → UI 回灌(避免循环)
    connect(control_bar_, &ControlBar::sgn_follow_system_toggled, playback_ctl_,
            &PlaybackController::set_follow_system_device);
    connect(playback_ctl_, &PlaybackController::sgn_follow_system_changed, control_bar_,
            &ControlBar::set_follow_system);
    // 初始同步(启动时按持久化状态勾选)
    control_bar_->set_follow_system(playback_ctl_->follow_system_device());

    connect(playback_ctl_, &PlaybackController::sgn_devices_changed, control_bar_,
            &ControlBar::set_device);
    connect(playback_ctl_, &PlaybackController::sgn_position_changed, control_bar_,
            &ControlBar::update_position);
    connect(playback_ctl_, &PlaybackController::sgn_playback_state_changed, control_bar_,
            &ControlBar::update_button_status);
    connect(playback_ctl_, &PlaybackController::sgn_duration_changed, control_bar_,
            &ControlBar::update_duration);
    connect(playlist_ctl_, &PlaylistController::sgn_play_mode_changed, this,
            [this](PlayMode mode) { control_bar_->set_play_mode(mode); });
    connect(playback_ctl_, &PlaybackController::sgn_volume_changed, control_bar_,
            &ControlBar::update_volume_slider);
    connect(playback_ctl_, &PlaybackController::sgn_mute_changed, control_bar_,
            &ControlBar::update_mute_button);

    connect(control_bar_, &ControlBar::sgn_btn_next_clicked, this,
            &PlaybackService::play_next_request);
    connect(control_bar_, &ControlBar::sgn_btn_prev_clicked, this,
            &PlaybackService::play_prev_request);

    connect(playback_ctl_, &PlaybackController::sgn_playback_natural_end, this, [this]() {
        const EntryId next_id = playlist_ctl_->next_track();
        if (!next_id.is_null()) {
            this->locate_on_next_play_request_ = true;
            const QString path                 = playlist_ctl_->track_file_path(next_id);
            if (!path.isEmpty()) {
                main_window_->play_track_in_ui(path);
            }
        }
    });

    // 请求播放:优先在当前播放列表定位(更新 context → song table/元数据同步);
    // 不在列表(库直播/外部)则按路径播放(面板走文件标签兜底)
    connect(playlist_ctl_, &PlaylistController::sgn_request_play, this,
            [this](const QString& filepath) {
                if (filepath.isEmpty()) {
                    return;
                }
                const bool located                 = playlist_ctl_->locate_filepath(filepath);
                this->locate_on_next_play_request_ = located; // 命中列表则定位高亮
                main_window_->play_track_in_ui(filepath);
            });

    bound_ = true;
}

void PlaybackService::start() {}

void PlaybackService::shutdown() {}

void PlaybackService::handle_play_track_request(const QString& filepath)
{
    MainWindow* main_window          = this->ctx_.main_window;
    PlaybackController* playback_ctl = this->ctx_.playback_controller;
    PlaylistController* playlist_ctl = this->ctx_.playlist_controller;

    if (filepath.isEmpty()) {
        locate_on_next_play_request_ = false;
        return;
    }

    auto* side_panel = main_window->side_panel();

    playback_ctl->read(filepath);
    side_panel->load_cover(filepath);

    if (locate_on_next_play_request_) {
        emit sgn_request_locate_current_track();
    }
    locate_on_next_play_request_ = false;

    TrackMetaData meta           = playlist_ctl->current_metadata();
    // 元数据必须匹配本次播放的文件;否则(库直播/外部/搜索播放)退回文件标签解析
    if (!meta.is_valid ||
        utils::path::normalize_path(meta.filepath) != utils::path::normalize_path(filepath)) {
        meta = utils::audio::parse_to_local_meta(filepath);
    }
    side_panel->load_lyrics(meta);
    side_panel->load_meta_data(meta);
}

void PlaybackService::play_track_request(const EntryId& eid)
{
    if (!eid.is_null()) {
        this->locate_on_next_play_request_ = true;
        const QString path                 = playlist_ctl_->track_file_path(eid);
        if (!path.isEmpty()) {
            main_window_->play_track_in_ui(path);
        }
    }
}

void PlaybackService::play_next_request()
{
    const EntryId next_id = playlist_ctl_->next_track();
    this->play_track_request(next_id);
}

void PlaybackService::play_prev_request()
{
    const EntryId prev_id = playlist_ctl_->prev_track();
    this->play_track_request(prev_id);
}

bool PlaybackService::is_playing()
{
    return (this->playback_ctl_->state() == PlayingState::Playing);
}
