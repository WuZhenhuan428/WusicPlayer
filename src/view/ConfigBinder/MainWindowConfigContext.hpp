#pragma once

// forward declare depencencies
class MainWindow;
class AppController;
class PlaybackController;
class PlaylistController;
class LibraryWidget;
class WControlBar;
class PlaylistSearchPanel;
class DesktopLyricsWidget;
class SettingsPanel;

class LibraryViewSection;
class PlaybackConfigSection;
class SearchPanelSection;
class WindowConfigSection;
class DesktopLyricsSection;
class SettingsPanelSection;

class MainWindowConfigContext
{
public:
    MainWindow* mainWindow{};
    AppController* appController{};
    PlaybackController* playbackController{};
    PlaylistController* playlistController{};
    LibraryWidget* libraryPanel{};
    WControlBar* controlBar{};
    PlaylistSearchPanel* searchPanel{};
    DesktopLyricsWidget* desktopLyrics{};
    SettingsPanel* settingsPanel{};

    WindowConfigSection* windowsSec{};
    PlaybackConfigSection* playbackSec{};
    LibraryViewSection* librarySec{};
    SearchPanelSection* searchSec{};
    DesktopLyricsSection* desktopSec{};
    SettingsPanelSection* settingsSec{};
};