add_library(wusic_view STATIC
    main_window.cpp
    main_window.h

    library_browser/library_browser_widget.cpp
    library_browser/library_browser_widget.h

    playlist_tree/playlist_tree_widget.cpp
    playlist_tree/playlist_tree_widget.h

    song_table/song_table_view.cpp
    song_table/song_table_view.h

    playlist/playlist_widgets.cpp
    playlist/playlist_widgets.h

    search_panel/search_panel.cpp
    search_panel/search_panel.h

    side_panel/elided_label.h
    side_panel/side_panel.cpp
    side_panel/side_panel.h
    side_panel/lyrics_search_widget.cpp
    side_panel/lyrics_search_widget.h
    side_panel/w_lyrics_model.cpp
    side_panel/w_lyrics_model.h
    side_panel/w_lyrics_panel.cpp
    side_panel/w_lyrics_panel.h

    control_bar/control_bar.cpp
    control_bar/control_bar.h
    control_bar/time_progress.cpp
    control_bar/time_progress.h

    desktop_lyrics_widget/desktop_lyrics_widget.cpp
    desktop_lyrics_widget/desktop_lyrics_widget.h

    settings_panel/settings_panel.cpp
    settings_panel/settings_panel.h

    settings_panel/shortcuts_panel/shortcuts_panel.cpp
    settings_panel/shortcuts_panel/shortcuts_panel.h

    settings_panel/lyrics_setting_panel/lyrics_setting_panel.cpp
    settings_panel/lyrics_setting_panel/lyrics_setting_panel.h

    settings_panel/theme_settings_page/theme_settings_page.cpp
    settings_panel/theme_settings_page/theme_settings_page.h

    settings_panel/library_settings_page.cpp
    settings_panel/library_settings_page.h

    settings_panel/plugin_panel/plugin_panel.cpp
    settings_panel/plugin_panel/plugin_panel.h

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

    dialogs/log_viewer_dialog.cpp
    dialogs/log_viewer_dialog.h
    
    dialogs/text_editor_dialog.cpp
    dialogs/text_editor_dialog.h

    dialogs/about_info_dialog.cpp
    dialogs/about_info_dialog.h
    
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
        Qt6::Svg
        magic_enum::magic_enum
        wusic_controller
        wusic_model
        wusic_core
        wusic_headers
        $<$<PLATFORM_ID:Windows>:dwmapi>
)