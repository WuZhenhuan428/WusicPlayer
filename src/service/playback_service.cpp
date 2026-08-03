#include "service/playback_service.h"

PlaybackService::PlaybackService(MainWindow* main_window, PlaybackController* playback_ctl,
                                 PlaylistController* playlist_ctl, QObject* parent) :
    QObject(parent), main_window_(main_window), m_playback_ctl(playback_ctl),
    m_playlist_ctl(playlist_ctl)
{
    assert(main_window_ && m_playback_ctl && m_playlist_ctl);
    m_control_bar = main_window_->control_bar_widget();
    assert(m_control_bar);
}

PlaybackService::~PlaybackService() {}

void PlaybackService::bind()
{
    if (m_bound) {
        return;
    }
    connect(main_window_, &MainWindow::sgnPlayTrackRequested, this,
            &PlaybackService::handle_play_track_request);
    connect(m_control_bar, &ControlBar::sgnBtnPlayPauseClicked, m_playback_ctl,
            [this](bool is_request_play) {
                if (is_request_play) {
                    m_playback_ctl->play();
                } else {
                    m_playback_ctl->pause();
                }
            });
    connect(m_control_bar, &ControlBar::sgnBtnStopClicked, m_playback_ctl,
            &PlaybackController::stop);
    connect(m_control_bar, &ControlBar::sgnBtnMuteClicked, m_playback_ctl,
            &PlaybackController::flip_mute);
    connect(m_control_bar, &ControlBar::sgnInOrder, this,
            [this]() { m_playlist_ctl->set_play_mode(PlayMode::in_order); });
    connect(m_control_bar, &ControlBar::sgnLoop, this,
            [this]() { m_playlist_ctl->set_play_mode(PlayMode::loop); });
    connect(m_control_bar, &ControlBar::sgnShuffle, this,
            [this]() { m_playlist_ctl->set_play_mode(PlayMode::shuffle); });
    connect(m_control_bar, &ControlBar::sgnOutOfOrderTrack, this,
            [this]() { m_playlist_ctl->set_play_mode(PlayMode::out_of_order_track); });
    connect(m_control_bar, &ControlBar::sgnOutOfOrderGroup, this,
            [this]() { m_playlist_ctl->set_play_mode(PlayMode::out_of_order_group); });
    connect(m_control_bar, &ControlBar::sgnSliderPositionReleased, this,
            [this](int percent) { m_playback_ctl->set_position(percent * 1000); });
    connect(m_control_bar, &ControlBar::sgnSliderVolumeReleased, m_playback_ctl,
            &PlaybackController::set_volume);
    connect(m_control_bar, &ControlBar::sgnSliderVolumeMoved, m_playback_ctl,
            &PlaybackController::set_volume);
    connect(m_control_bar, &ControlBar::sgnSelectDeviceId, m_playback_ctl,
            &PlaybackController::set_device_by_id);

    connect(m_playback_ctl, &PlaybackController::sgn_devices_changed, m_control_bar,
            &ControlBar::set_device);
    connect(m_playback_ctl, &PlaybackController::sgn_position_changed, m_control_bar,
            &ControlBar::update_position);
    connect(m_playback_ctl, &PlaybackController::sgn_playback_state_changed, m_control_bar,
            &ControlBar::update_button_status);
    connect(m_playback_ctl, &PlaybackController::sgn_duration_changed, m_control_bar,
            &ControlBar::update_duration);
    connect(m_playlist_ctl, &PlaylistController::sgn_play_mode_changed, this,
            [this](PlayMode mode) { m_control_bar->set_play_mode(mode); });
    connect(m_playback_ctl, &PlaybackController::sgn_volume_changed, m_control_bar,
            &ControlBar::update_volume_slider);
    connect(m_playback_ctl, &PlaybackController::sgn_mute_changed, m_control_bar,
            &ControlBar::update_mute_button);

    connect(m_control_bar, &ControlBar::sgnBtnNextClicked, this, [this]() {
        const EntryId next_id = m_playlist_ctl->next_track();
        if (!next_id.isNull()) {
            locate_on_next_play_request_ = true;
            const QString path           = m_playlist_ctl->track_file_path(next_id);
            if (!path.isEmpty()) {
                main_window_->play_track_in_ui(path);
            }
        }
    });

    connect(m_control_bar, &ControlBar::sgnBtnPrevClicked, this, [this]() {
        const EntryId prev_id = m_playlist_ctl->prev_track();
        if (!prev_id.isNull()) {
            locate_on_next_play_request_ = true;
            const QString path           = m_playlist_ctl->track_file_path(prev_id);
            if (!path.isEmpty()) {
                main_window_->play_track_in_ui(path);
            }
        }
    });

    connect(m_playback_ctl, &PlaybackController::sgn_playback_natural_end, this, [this]() {
        const EntryId next_id = m_playlist_ctl->next_track();
        if (!next_id.isNull()) {
            locate_on_next_play_request_ = true;
            const QString path           = m_playlist_ctl->track_file_path(next_id);
            if (!path.isEmpty()) {
                main_window_->play_track_in_ui(path);
            }
        }
    });

    connect(m_playlist_ctl, &PlaylistController::sgn_request_play, this,
            [this](const QString& filepath) { main_window_->play_track_in_ui(filepath); });

    m_bound = true;
}

void PlaybackService::start() {}

void PlaybackService::shutdown() {}

void PlaybackService::handle_play_track_request(const QString& filepath)
{
    if (filepath.isEmpty()) {
        locate_on_next_play_request_ = false;
        return;
    }

    auto* side_panel = main_window_->side_panel();

    m_playback_ctl->read(filepath);
    side_panel->load_cover(filepath);

    if (locate_on_next_play_request_) {
        emit sgnLocateCurrentTrack();
    }
    locate_on_next_play_request_ = false;

    TrackMetaData meta           = m_playlist_ctl->current_metadata();
    side_panel->load_lyrics(meta);
    side_panel->load_meta_data(meta);
}
