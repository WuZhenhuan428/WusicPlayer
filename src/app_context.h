#pragma once

class MainWindow;
class PlaybackController;
class ThemeService;
class InMemorySearchBackend;
class LogSinkGui;
class PlaylistController;
class LibraryManager;
class PlaylistManager;
class PluginController;
class EventBus;

/**
 * @brief 用于聚合 PanelCoordinator 中非持有的资源, 在组合根 (AppCpntroller) 中一次性构建
 */
struct AppContext
{
    MainWindow* main_window                         = nullptr;
    PlaybackController* playback_controller         = nullptr;
    PlaylistController* playlist_controller         = nullptr;
    PlaylistManager* playlist_manager               = nullptr;
    ThemeService* theme_service                     = nullptr;
    InMemorySearchBackend* in_memory_search_backend = nullptr;
    LogSinkGui* log_sink_gui                        = nullptr;
    LibraryManager* library_manager                 = nullptr;
    PluginController* plugin_controller             = nullptr;
    EventBus* event_bus                             = nullptr;
};
