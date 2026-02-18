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
        QByteArray state;
    } view;

    struct SearchPanel
    {
        QByteArray geometry;
        QByteArray state;
    } search_panel;

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

    const AppConfig& getAppConfig() const;
    const AppConfig::WindowState& getWindowState() const;
    const AppConfig::PlaybackState& getPlaybackState() const;
    const AppConfig::ViewSettings& getViewState() const;
    const AppConfig::SearchPanel& getSearchPanelState() const;
    
    void setWindowGeometry(const QByteArray& geo);
    void setWindowState(const QByteArray& state);
    void setVolume(int volume);
    void setMute(bool is_mute);
    // setter: playback
    void setLastPlayInfo(const QUuid& playlist_id, const QUuid& track_id, int position_ms);
    void setPlayMode(PlayMode mode);
    // setter: view
    void setTableColumns(const QList<TableColumn>& columns);
    void setSongTreeViewHeader(QByteArray header);
    // setter: search_panel
    void setSearchPanelGeometry(QByteArray geo);
    void setSearchPanelHeader(QByteArray header);
private:
    ConfigManager();
    ~ConfigManager() = default;
    
    QString getConfigPath() const;
    QString getConfigFilepath() const;

    AppConfig m_appConfig;
};