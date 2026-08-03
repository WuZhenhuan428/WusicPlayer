#include "playback_service.h"

PlaybackService::PlaybackService(MainWindow* main_window, PlaybackController* playback_ctl,
                                 PlaylistController* playlist_ctl, QObject* parent) :
    QObject(parent), main_window_(main_window), m_playback_ctl(playback_ctl),
    m_playlist_ctl(playlist_ctl)
{
    assert(main_window_ && m_playback_ctl && m_playlist_ctl);
    m_control_bar = main_window_->controlBarWidget();
    assert(m_control_bar);
}

PlaybackService::~PlaybackService() {}

void PlaybackService::bind()
{
    if (m_bound) {
        return;
    }
    connect(main_window_, &MainWindow::sgnPlayTrackRequested, this,
            &PlaybackService::handlePlayTrackRequest);
    connect(m_control_bar, &WControlBar::sgnBtnPlayPauseClicked, m_playback_ctl,
            [this](bool is_request_play) {
                if (is_request_play) {
                    m_playback_ctl->play();
                } else {
                    m_playback_ctl->pause();
                }
            });
    connect(m_control_bar, &WControlBar::sgnBtnStopClicked, m_playback_ctl,
            &PlaybackController::stop);
    connect(m_control_bar, &WControlBar::sgnBtnMuteClicked, m_playback_ctl,
            &PlaybackController::flipMute);
    connect(m_control_bar, &WControlBar::sgnInOrder, this,
            [this]() { m_playlist_ctl->setPlayMode(PlayMode::in_order); });
    connect(m_control_bar, &WControlBar::sgnLoop, this,
            [this]() { m_playlist_ctl->setPlayMode(PlayMode::loop); });
    connect(m_control_bar, &WControlBar::sgnShuffle, this,
            [this]() { m_playlist_ctl->setPlayMode(PlayMode::shuffle); });
    connect(m_control_bar, &WControlBar::sgnOutOfOrderTrack, this,
            [this]() { m_playlist_ctl->setPlayMode(PlayMode::out_of_order_track); });
    connect(m_control_bar, &WControlBar::sgnOutOfOrderGroup, this,
            [this]() { m_playlist_ctl->setPlayMode(PlayMode::out_of_order_group); });
    connect(m_control_bar, &WControlBar::sgnSliderPositionReleased, this,
            [this](int percent) { m_playback_ctl->setPosition(percent * 1000); });
    connect(m_control_bar, &WControlBar::sgnSliderVolumeReleased, m_playback_ctl,
            &PlaybackController::setVolume);
    connect(m_control_bar, &WControlBar::sgnSliderVolumeMoved, m_playback_ctl,
            &PlaybackController::setVolume);
    connect(m_control_bar, &WControlBar::sgnSelectDeviceId, m_playback_ctl,
            &PlaybackController::setDeviceById);

    connect(m_playback_ctl, &PlaybackController::sgnDevicesChanged, m_control_bar,
            &WControlBar::setDevice);
    connect(m_playback_ctl, &PlaybackController::sgnPositionChanged, m_control_bar,
            &WControlBar::updatePosition);
    connect(m_playback_ctl, &PlaybackController::sgnPlaybackStateChanged, m_control_bar,
            &WControlBar::updateButtonStatus);
    connect(m_playback_ctl, &PlaybackController::sgnDurationChanged, m_control_bar,
            &WControlBar::updateDuration);
    connect(m_playlist_ctl, &PlaylistController::playModeChanged, this,
            [this](PlayMode mode) { m_control_bar->setPlayMode(mode); });
    connect(m_playback_ctl, &PlaybackController::sgnVolumeChanged, m_control_bar,
            &WControlBar::updateVolumeSlider);
    connect(m_playback_ctl, &PlaybackController::sgnMuteChanged, m_control_bar,
            &WControlBar::updateMuteButton);

    connect(m_control_bar, &WControlBar::sgnBtnNextClicked, this, [this]() {
        const EntryId next_id = m_playlist_ctl->nextTrack();
        if (!next_id.isNull()) {
            locate_on_next_play_request_ = true;
            const QString path           = m_playlist_ctl->trackFilePath(next_id);
            if (!path.isEmpty()) {
                main_window_->playTrackInUi(path);
            }
        }
    });

    connect(m_control_bar, &WControlBar::sgnBtnPrevClicked, this, [this]() {
        const EntryId prev_id = m_playlist_ctl->prevTrack();
        if (!prev_id.isNull()) {
            locate_on_next_play_request_ = true;
            const QString path           = m_playlist_ctl->trackFilePath(prev_id);
            if (!path.isEmpty()) {
                main_window_->playTrackInUi(path);
            }
        }
    });

    connect(m_playback_ctl, &PlaybackController::sgnPlaybackNatualEnd, this, [this]() {
        const EntryId next_id = m_playlist_ctl->nextTrack();
        if (!next_id.isNull()) {
            locate_on_next_play_request_ = true;
            const QString path           = m_playlist_ctl->trackFilePath(next_id);
            if (!path.isEmpty()) {
                main_window_->playTrackInUi(path);
            }
        }
    });

    connect(m_playlist_ctl, &PlaylistController::requestPlay, this,
            [this](const QString& filepath) { main_window_->playTrackInUi(filepath); });

    m_bound = true;
}

void PlaybackService::start() {}

void PlaybackService::shutdown() {}

void PlaybackService::handlePlayTrackRequest(const QString& filepath)
{
    if (filepath.isEmpty()) {
        locate_on_next_play_request_ = false;
        return;
    }

    auto* sidePanel = main_window_->sidePanel();

    m_playback_ctl->read(filepath);
    sidePanel->loadCover(filepath);

    if (locate_on_next_play_request_) {
        emit sgnLocateCurrentTrack();
    }
    locate_on_next_play_request_ = false;

    TrackMetaData meta           = m_playlist_ctl->currentMetadata();
    sidePanel->loadLyrics(meta);
    sidePanel->loadMetaData(meta);
}
