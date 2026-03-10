#pragma once

// forward declare depencencies
class MainWindow;
class PlaybackController;
class PlaylistController;
class LibraryWidget;
class WControlBar;
class PlaylistSearchPanel;
class DesktopLyricsWidget;

class LibraryViewSection;
class PlaybackConfigSection;
class SearchPanelSection;
class WindowConfigSection;
class DesktopLyricsSection;

class MainWindowConfigContext
{
public:
    MainWindow* mainWindow{};
    PlaybackController* playbackController{};
    PlaylistController* playlistController{};
    LibraryWidget* libraryPanel{};
    WControlBar* controlBar{};
    PlaylistSearchPanel* searchPanel{};
    DesktopLyricsWidget* desktopLyrics{};

    WindowConfigSection* windowsSec{};
    PlaybackConfigSection* playbackSec{};
    LibraryViewSection* librarySec{};
    SearchPanelSection* searchSec{};
    DesktopLyricsSection* desktopSec{};
};