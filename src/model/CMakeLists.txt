add_library(wusic_model STATIC
    player/player.h
    player/player.cpp
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
)

target_link_libraries(wusic_model
    PUBLIC
        Qt6::Core
        Qt6::Multimedia
        wusic_core
        wusic_headers
        lrc_parser
        PkgConfig::TAGLIB
)