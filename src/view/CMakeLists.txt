add_library(wusic_view STATIC
    
    MainWindow.cpp
    MainWindow.h

    LibraryWidget/LibraryWidget.cpp
    LibraryWidget/LibraryWidget.h

    playlist/playlist_widgets.cpp
    playlist/playlist_widgets.h

    search_panel/playlist_search_panel.cpp
    search_panel/playlist_search_panel.h

    SidePanel/ElidedLabel.h
    SidePanel/SidePanel.cpp
    SidePanel/SidePanel.h
    SidePanel/WLyricsModel.cpp
    SidePanel/WLyricsModel.h
    SidePanel/WLyricsPanel.cpp
    SidePanel/WLyricsPanel.h

    WControlBar/WControlBar.cpp
    WControlBar/WControlBar.h
    WControlBar/wtimeprogress.cpp
    WControlBar/wtimeprogress.h

    DesktopLyricsWidget/DesktopLyricsWidget.cpp
    DesktopLyricsWidget/DesktopLyricsWidget.h

    ConfigBinder/IConfigSection.hpp
    ConfigBinder/LibraryViewSection.hpp
    ConfigBinder/PlaybackConfigSection.hpp
    ConfigBinder/SearchPanelSection.hpp
    ConfigBinder/WindowConfigSection.hpp
    ConfigBinder/DesktopLyricsSection.hpp
    ConfigBinder/MainWindowConfigContext.hpp
    ConfigBinder/IConfigBinder.hpp
    ConfigBinder/LibraryViewBinder.hpp
    ConfigBinder/LibraryViewBinder.cpp
    ConfigBinder/PlaybackConfigBinder.hpp
    ConfigBinder/PlaybackConfigBinder.cpp
    ConfigBinder/SearchPanelBinder.hpp
    ConfigBinder/SearchPanelBinder.cpp
    ConfigBinder/WindowConfigBinder.hpp
    ConfigBinder/WindowConfigBinder.cpp
    ConfigBinder/DesktopLyricsBinder.hpp
    ConfigBinder/DesktopLyricsBinder.cpp
)

target_link_libraries(wusic_view
    PUBLIC
        Qt6::Core
        Qt6::Widgets
        Qt6::Multimedia
        wusic_controller
        wusic_model
        wusic_core
        wusic_headers
)