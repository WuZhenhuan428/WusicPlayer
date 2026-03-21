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

    PlaybackRestoreCoordinator.hpp

    ConfigBinder/MainWindowConfigContext.hpp

    ConfigBinder/IConfigSection.hpp
    ConfigBinder/LibraryViewSection.hpp
    ConfigBinder/PlaybackConfigSection.hpp
    ConfigBinder/SearchPanelSection.hpp
    ConfigBinder/WindowConfigSection.hpp
    ConfigBinder/DesktopLyricsSection.hpp
    ConfigBinder/SettingsPanelSection.hpp
    ConfigBinder/ShortcutsSection.hpp
    
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
    ConfigBinder/SettingsPanelBinder.hpp
    ConfigBinder/SettingsPanelBinder.cpp
    ConfigBinder/ShortcutsBinder.hpp
    ConfigBinder/ShortcutsBinder.cpp

    SettingsPanel/SettingsPanel.cpp
    SettingsPanel/SettingsPanel.hpp

    SettingsPanel/ShortcutsPanel/ShortcutsPanel.cpp
    SettingsPanel/ShortcutsPanel/ShortcutsPanel.hpp

    SettingsPanel/lyrics_setting_panel/lyrics_setting_panel.cpp
    SettingsPanel/lyrics_setting_panel/lyrics_setting_panel.h

    dialogs/hsv_dialog/basic_colors_item.cpp
    dialogs/hsv_dialog/basic_colors_item.h
    dialogs/hsv_dialog/color_preview_item.cpp
    dialogs/hsv_dialog/color_preview_item.h
    dialogs/hsv_dialog/hsv_dialog.cpp
    dialogs/hsv_dialog/hsv_dialog.h
    dialogs/hsv_dialog/hsv_palette.cpp
    dialogs/hsv_dialog/hsv_palette.h
    dialogs/hsv_dialog/hsv_to_rgb.c
    dialogs/hsv_dialog/hsv_to_rgb.h
    dialogs/hsv_dialog/label_matrix.cpp
    dialogs/hsv_dialog/label_matrix.h

    dialogs/font_select_dialog.cpp
    dialogs/font_select_dialog.h
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