#include "service/playback_service.h"

#include "core/utils/audio.hpp"

#include "app_context.h"
#include "controller/playback_controller.h"
#include "controller/playlist_controller.h"
#include "view/control_bar/control_bar.h"
#include "view/main_window.h"

PlaybackService::PlaybackService(AppContext& ctx, QObject* parent) : QObject(parent), ctx_(ctx)
{
    assert(ctx_.main_window_ && ctx_.playback_controller_ && ctx.playlist_controller_);
    assert(ctx_.main_window_->control_bar_widget());

    this->main_window  = this->ctx_.main_window_;
    this->control_bar  = this->ctx_.main_window_->control_bar_widget();
    this->playback_ctl = this->ctx_.playback_controller_;
    this->playlist_ctl = this->ctx_.playlist_controller_;
}

PlaybackService::~PlaybackService() {}

void PlaybackService::bind()
{
    if (m_bound) {
        return;
    }
    connect(ctx_.main_window_, &MainWindow::sgnPlayTrackRequested, this,
            &PlaybackService::handle_play_track_request);
    connect(control_bar, &ControlBar::sgnBtnPlayPauseClicked, playback_ctl,
            [this](bool is_request_play) {
                if (is_request_play) {
                    playback_ctl->play();
                } else {
                    playback_ctl->pause();
                }
            });
    connect(control_bar, &ControlBar::sgnBtnStopClicked, playback_ctl, &PlaybackController::stop);
    connect(control_bar, &ControlBar::sgnBtnMuteClicked, playback_ctl,
            &PlaybackController::flip_mute);
    connect(control_bar, &ControlBar::sgnInOrder, this,
            [this]() { playlist_ctl->set_play_mode(PlayMode::in_order); });
    connect(control_bar, &ControlBar::sgnLoop, this,
            [this]() { playlist_ctl->set_play_mode(PlayMode::loop); });
    connect(control_bar, &ControlBar::sgnShuffle, this,
            [this]() { playlist_ctl->set_play_mode(PlayMode::shuffle); });
    connect(control_bar, &ControlBar::sgnOutOfOrderTrack, this,
            [this]() { playlist_ctl->set_play_mode(PlayMode::out_of_order_track); });
    connect(control_bar, &ControlBar::sgnOutOfOrderGroup, this,
            [this]() { playlist_ctl->set_play_mode(PlayMode::out_of_order_group); });
    connect(control_bar, &ControlBar::sgnSliderPositionReleased, this,
            [this](int percent) { playback_ctl->set_position(percent * 1000); });
    connect(control_bar, &ControlBar::sgnSliderVolumeReleased, playback_ctl,
            &PlaybackController::set_volume);
    connect(control_bar, &ControlBar::sgnSliderVolumeMoved, playback_ctl,
            &PlaybackController::set_volume);
    connect(control_bar, &ControlBar::sgnSelectDeviceId, playback_ctl,
            &PlaybackController::set_device_by_id);

    connect(playback_ctl, &PlaybackController::sgn_devices_changed, control_bar,
            &ControlBar::set_device);
    connect(playback_ctl, &PlaybackController::sgn_position_changed, control_bar,
            &ControlBar::update_position);
    connect(playback_ctl, &PlaybackController::sgn_playback_state_changed, control_bar,
            &ControlBar::update_button_status);
    connect(playback_ctl, &PlaybackController::sgn_duration_changed, control_bar,
            &ControlBar::update_duration);
    connect(playlist_ctl, &PlaylistController::sgn_play_mode_changed, this,
            [this](PlayMode mode) { control_bar->set_play_mode(mode); });
    connect(playback_ctl, &PlaybackController::sgn_volume_changed, control_bar,
            &ControlBar::update_volume_slider);
    connect(playback_ctl, &PlaybackController::sgn_mute_changed, control_bar,
            &ControlBar::update_mute_button);

    connect(control_bar, &ControlBar::sgnBtnNextClicked, this, &PlaybackService::play_next_request);
    connect(control_bar, &ControlBar::sgnBtnPrevClicked, this, &PlaybackService::play_prev_request);

    connect(playback_ctl, &PlaybackController::sgn_playback_natural_end, this, [this]() {
        const EntryId next_id = playlist_ctl->next_track();
        if (!next_id.is_null()) {
            this->locate_on_next_play_request_ = true;
            const QString path                 = playlist_ctl->track_file_path(next_id);
            if (!path.isEmpty()) {
                main_window->play_track_in_ui(path);
            }
        }
    });

    // 请求播放:优先在当前播放列表定位(更新 context → song table/元数据同步);
    // 不在列表(库直播/外部)则按路径播放(面板走文件标签兜底)
    connect(playlist_ctl, &PlaylistController::sgn_request_play, this,
            [this](const QString& filepath) {
                if (filepath.isEmpty()) {
                    return;
                }
                const bool located                 = playlist_ctl->locate_filepath(filepath);
                this->locate_on_next_play_request_ = located; // 命中列表则定位高亮
                main_window->play_track_in_ui(filepath);
            });

    m_bound = true;
}

void PlaybackService::start() {}

void PlaybackService::shutdown() {}

void PlaybackService::handle_play_track_request(const QString& filepath)
{
    MainWindow* main_window          = this->ctx_.main_window_;
    PlaybackController* playback_ctl = this->ctx_.playback_controller_;
    PlaylistController* playlist_ctl = this->ctx_.playlist_controller_;

    if (filepath.isEmpty()) {
        locate_on_next_play_request_ = false;
        return;
    }

    auto* side_panel = main_window->side_panel();

    playback_ctl->read(filepath);
    side_panel->load_cover(filepath);

    if (locate_on_next_play_request_) {
        emit sgnLocateCurrentTrack();
    }
    locate_on_next_play_request_ = false;

    TrackMetaData meta           = playlist_ctl->current_metadata();
    // 元数据必须匹配本次播放的文件;否则(库直播/外部/搜索播放)退回文件标签解析
    if (!meta.isValid ||
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
        const QString path                 = playlist_ctl->track_file_path(eid);
        if (!path.isEmpty()) {
            main_window->play_track_in_ui(path);
        }
    }
}

void PlaybackService::play_next_request()
{
    const EntryId next_id = playlist_ctl->next_track();
    this->play_track_request(next_id);
}

void PlaybackService::play_prev_request()
{
    const EntryId prev_id = playlist_ctl->prev_track();
    this->play_track_request(prev_id);
}

bool PlaybackService::is_playing()
{
    return (this->playback_ctl->state() == PlayingState::PLAYING);
}
