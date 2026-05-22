add_library(wusic_view STATIC
    MainWindow.cpp
    MainWindow.h

    LibraryWidget/LibraryWidget.cpp
    LibraryWidget/LibraryWidget.h

    playlist/playlist_widgets.cpp
    playlist/playlist_widgets.h

    search_panel/search_panel.cpp
    search_panel/search_panel.h

    SidePanel/ElidedLabel.h
    SidePanel/SidePanel.cpp
    SidePanel/SidePanel.h
    SidePanel/LyricsSearchWidget.cpp
    SidePanel/LyricsSearchWidget.h
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

    SettingsPanel/SettingsPanel.cpp
    SettingsPanel/SettingsPanel.h

    SettingsPanel/ShortcutsPanel/ShortcutsPanel.cpp
    SettingsPanel/ShortcutsPanel/ShortcutsPanel.h

    SettingsPanel/lyrics_setting_panel/lyrics_setting_panel.cpp
    SettingsPanel/lyrics_setting_panel/lyrics_setting_panel.h

    SettingsPanel/ThemeSettingsPage/ThemeSettingsPage.cpp
    SettingsPanel/ThemeSettingsPage/ThemeSettingsPage.h

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

    dialogs/text_editor_dialog.cpp
    dialogs/text_editor_dialog.h

    tag_edit_widget/tag_edit_widget.cpp
    tag_edit_widget/tag_edit_widget.h
    tag_edit_widget/new_tag_item_dialog.cpp
    tag_edit_widget/new_tag_item_dialog.h

    eq_widget/eq_widget.cpp
    eq_widget/eq_widget.h
)

target_link_libraries(wusic_view
    PUBLIC
        Qt6::Core
        Qt6::Widgets
        Qt6::Multimedia
        Qt6::Svg
        magic_enum::magic_enum
        wusic_controller
        wusic_model
        wusic_core
        wusic_headers
        $<$<PLATFORM_ID:Windows>:dwmapi>
)