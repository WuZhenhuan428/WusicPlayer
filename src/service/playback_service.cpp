#include "playback_service.h"

PlaybackService::PlaybackService(MainWindow* main_window, PlaybackController* playback_ctl,
                             PlaylistController* playlist_ctl, QObject *parent)
    : QObject(parent),
      m_main_window(main_window),
      m_playback_ctl(playback_ctl),
      m_playlist_ctl(playlist_ctl)
{
    assert(m_main_window && m_playback_ctl && m_playlist_ctl);
    m_control_bar = m_main_window->controlBarWidget();
    assert(m_control_bar);
}

PlaybackService::~PlaybackService() {}

void PlaybackService::bind()
{
    if (m_bound) {
        return;
    }
    connect(m_main_window, &MainWindow::sgnPlayTrackRequested, this, &PlaybackService::handlePlayTrackRequest);
    connect(m_control_bar, &WControlBar::sgnBtnPlayClicked, m_playback_ctl, &PlaybackController::play);
    connect(m_control_bar, &WControlBar::sgnBtnPauseClicked, m_playback_ctl, &PlaybackController::pause);
    connect(m_control_bar, &WControlBar::sgnBtnStopClicked, m_playback_ctl, &PlaybackController::stop);
    connect(m_control_bar, &WControlBar::sgnBtnMuteClicked, m_playback_ctl, &PlaybackController::flipMute);
    connect(m_control_bar, &WControlBar::sgnInOrder, this, [this]() {
        m_playback_ctl->setPlayMode(PlayMode::in_order);
    });
    connect(m_control_bar, &WControlBar::sgnLoop, this, [this]() {
        m_playback_ctl->setPlayMode(PlayMode::loop);
    });
    connect(m_control_bar, &WControlBar::sgnShuffle, this, [this]() {
        m_playback_ctl->setPlayMode(PlayMode::shuffle);
    });
    connect(m_control_bar, &WControlBar::sgnOutOfOrderTrack, this, [this]() {
        m_playback_ctl->setPlayMode(PlayMode::out_of_order_track);
    });
    connect(m_control_bar, &WControlBar::sgnOutOfOrderGroup, this, [this]() {
        m_playback_ctl->setPlayMode(PlayMode::out_of_order_group);
    });
    connect(m_control_bar, &WControlBar::sgnSliderPositionReleased, this, [this](int percent) {
        m_playback_ctl->setPosition(percent * 1000);
    });
    connect(m_control_bar, &WControlBar::sgnSliderVolumeReleased, m_playback_ctl, &PlaybackController::setVolume);
    connect(m_control_bar, &WControlBar::sgnSliderVolumeMoved, m_playback_ctl, &PlaybackController::setVolume);
    connect(m_control_bar, &WControlBar::sgnSelectDeviceId, m_playback_ctl, &PlaybackController::setDeviceById);

    connect(m_playback_ctl, &PlaybackController::sgnDevicesChanged, m_control_bar, &WControlBar::setDevice);
    connect(m_playback_ctl, &PlaybackController::sgnPositionChanged, m_control_bar, &WControlBar::updatePosition);
    connect(m_playback_ctl, &PlaybackController::sgnPlaybackStateChanged, this, [this](PlayingState state) {
        QMediaPlayer::PlaybackState ui_state = QMediaPlayer::PlaybackState::StoppedState;
        if (state == PlayingState::PLAYING) {
            ui_state = QMediaPlayer::PlaybackState::PlayingState;
        } else if (state == PlayingState::PAUSE) {
            ui_state = QMediaPlayer::PlaybackState::PausedState;
        }
        m_control_bar->onPlayerStateChanged(ui_state);
    });
    connect(m_playback_ctl, &PlaybackController::sgnDurationChanged, m_control_bar, &WControlBar::updateDuration);
    connect(m_playback_ctl, &PlaybackController::sgnPlayModeChanged, this, [this](PlayMode mode) {
        m_control_bar->setPlayMode(mode);
    });

    connect(m_control_bar, &WControlBar::sgnBtnNextClicked, this, [this]() {
        QString nextTrack = m_playlist_ctl->nextTrack(m_playback_ctl->playMode());
        if (!nextTrack.isEmpty()) {
            m_locate_on_next_play_request = true;
            m_main_window->playTrackInUi(nextTrack);
        }
    });

    connect(m_control_bar, &WControlBar::sgnBtnPrevClicked, this, [this]() {
        QString prevTrack = m_playlist_ctl->prevTrack(m_playback_ctl->playMode());
        if (!prevTrack.isEmpty()) {
            m_locate_on_next_play_request = true;
            m_main_window->playTrackInUi(prevTrack);
        }
    });

    connect(m_playback_ctl, &PlaybackController::sgnPlaybackFinished,
            this, [this]() {
        QString nextTrack = m_playlist_ctl->nextTrack(m_playback_ctl->playMode());
        if (!nextTrack.isEmpty()) {
            m_locate_on_next_play_request = true;
            m_main_window->playTrackInUi(nextTrack);
        }
    });

    connect(m_playlist_ctl, &PlaylistController::requestPlay,
            this, [this](const QString& filepath) {
        m_main_window->playTrackInUi(filepath);
    });

    m_bound = true;
}

void PlaybackService::start()
{

}

void PlaybackService::shutdown()
{

}


void PlaybackService::handlePlayTrackRequest(const QString &filepath)
{
    if (filepath.isEmpty()) {
        m_locate_on_next_play_request = false;
        return;
    }

    auto* sidePanel = m_main_window->sidePanel();

    m_playback_ctl->read(filepath);
    sidePanel->loadCover(filepath);

    if (m_locate_on_next_play_request) {
        emit sgnLocateCurrentTrack();
    }
    m_locate_on_next_play_request = false;

    TrackMetaData meta = m_playlist_ctl->currentMetadata();
    sidePanel->loadLyrics(meta);
    sidePanel->loadMetaData(meta);
}