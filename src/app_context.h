#pragma once

class MainWindow;
class PlaybackController;
class ThemeService;
class InMemorySearchBackend;
class LogSinkGui;
class PlaylistController;
class LibraryManager;
class PlaylistManager;
class EventBus;

/**
 * @brief 用于聚合 PanelCoordinator 中非持有的资源, 在组合根 (AppCpntroller) 中一次性构建
 */
class AppContext
{
public:
    MainWindow* main_window_                         = nullptr;
    PlaybackController* playback_controller_         = nullptr;
    PlaylistController* playlist_controller_         = nullptr;
    PlaylistManager* playlist_manager_               = nullptr;
    ThemeService* theme_service_                     = nullptr;
    InMemorySearchBackend* in_memory_search_backend_ = nullptr;
    LogSinkGui* log_sink_gui_                        = nullptr;
    LibraryManager* library_manager_                 = nullptr;
    EventBus* event_bus_                             = nullptr;
};
