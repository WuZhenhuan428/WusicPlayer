#pragma once

#include <QByteArray>
#include <QUuid>
#include "../playlist/playlist_definitions.h"

// map config to json
struct AppConfig {
    struct WindowState
    {
        QByteArray geometry;
        QByteArray state;
        int volume;
        bool isMuted;
    } window;

    struct PlaybackState
    {
        QUuid last_playlist_id;
        QUuid last_track_id;
        int position_ms;
        PlayMode play_mode;
    } playback;
    
    struct ViewSettings
    {
        QList<TableColumn> columns;
    } view;
};

class ConfigManager
{
public:
    // delete constructor
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    // global access entry
    static ConfigManager& getInstance() {
        static ConfigManager instance;
        return instance;
    }

private:
    ConfigManager() {
        // init & read configurations
    }
    ~ConfigManager() {
        // save configurations
    }

    QString getConfigPath();
    QString parseConfigFile(QString filepath);

    void load();
    void save();

    AppConfig m_appConfig;
public:     // public interface
    const AppConfig& getAppConfig() { return m_appConfig; }
    const AppConfig::WindowState& getWindowState() { return m_appConfig.window; }
    const AppConfig::PlaybackState& getPlaybackState() { return m_appConfig.playback; }
    const AppConfig::ViewSettings& getViewState() { return m_appConfig.view; }

    // setter: window
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
};