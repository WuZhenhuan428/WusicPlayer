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