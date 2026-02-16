#pragma once

#include <QByteArray>
#include <QList>
#include <QString>
#include <QUuid>
#include "../playlist/playlist_definitions.h"

// map config to json
struct AppConfig {
    struct WindowState
    {
        QByteArray geometry;
        QByteArray state;
        int volume = 100;
        bool isMuted = false;
    } window;

    struct PlaybackState
    {
        QUuid last_playlist_id;
        QUuid last_track_id;
        int position_ms = 0;
        PlayMode play_mode = PlayMode::in_order;
    } playback;
    
    struct ViewSettings
    {
        QList<TableColumn> columns;
        QByteArray song_tree_header_state;
    } view;

    int version = 1;
};

class ConfigManager
{
public:
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    static ConfigManager& getInstance() {
        static ConfigManager instance;
        return instance;
    }
    void load();
    void save();
    void setDefault();

    const AppConfig& getAppConfig() const { return m_appConfig; }
    const AppConfig::WindowState& getWindowState() const { return m_appConfig.window; };
    const AppConfig::PlaybackState& getPlaybackState() const { return m_appConfig.playback; }
    const AppConfig::ViewSettings& getViewState() const { return m_appConfig.view; }
    
    void setWindowGeometry(const QByteArray& geo) { m_appConfig.window.geometry = geo; }
    void setWindowState(const QByteArray& state) { m_appConfig.window.state = state; }
    void setVolume(int volume) { m_appConfig.window.volume = volume; }
    void setMute(bool is_mute) { m_appConfig.window.isMuted= is_mute; }
    // setter: playback
    void setLastPlayInfo(const QUuid& playlist_id, const QUuid& track_id, int position_ms) {
        m_appConfig.playback.last_playlist_id = playlist_id;
        m_appConfig.playback.last_track_id = track_id;
        m_appConfig.playback.position_ms = position_ms;
    }
    void setPlayMode(PlayMode mode) {
        m_appConfig.playback.play_mode = mode;
    }
    // setter: view
    void setTableColumns(const QList<TableColumn>& columns) {
        m_appConfig.view.columns = columns;
    }
    void setSongTreeViewHeader(QByteArray header) {
        m_appConfig.view.song_tree_header_state = header;
    }
private:
    ConfigManager();
    ~ConfigManager() = default;
    
    QString getConfigPath() const;
    QString getConfigFilepath() const;

    AppConfig m_appConfig;
};