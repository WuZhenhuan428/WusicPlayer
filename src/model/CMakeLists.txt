add_library(wusic_model STATIC
    library/library.cpp
    library/library.h
    library/library_track.h
    library/library_repo.cpp
    library/library_repo.h
    library/library_scanner.cpp
    library/library_scanner.h
    library/library_file_watcher.cpp
    library/library_file_watcher.h
    library/library_manager.cpp
    library/library_manager.h
    library/library_browse_model.cpp
    library/library_browse_model.h

    playback_queue/queue_item.h
    playback_queue/playback_queue.cpp
    playback_queue/playback_queue.h
    playback_queue/playback_queue_service.cpp
    playback_queue/playback_queue_service.h

    playlist/playlist_context.cpp
    playlist/playlist_context.h
    playlist/playlist.cpp
    playlist/playlist.h
    playlist/playlist_layout.cpp
    playlist/playlist_layout.h
    playlist/playlist_manager.cpp
    playlist/playlist_manager.h
    playlist/playlist_repo.cpp
    playlist/playlist_repo.h
    playlist/playlist_view_model.cpp
    playlist/playlist_view_model.h

    ShortcutsViewModel/shortcuts_view_model.cpp
    ShortcutsViewModel/shortcuts_view_model.hpp
    ShortcutsViewModel/shortcuts_types.hpp

    search_model/search_model.cpp
    search_model/search_model.h

    theme_settings_model.h
    theme_settings_model.cpp
)

target_link_libraries(wusic_model
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Sql
        wusic_core
        wusic_headers
        lrc_parser
        Taglib::Taglib
)