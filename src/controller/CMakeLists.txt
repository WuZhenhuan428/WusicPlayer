add_library(wusic_controller STATIC
    playback_controller.h
    playback_controller.cpp

    playlist_controller.h
    playlist_controller.cpp

    search_backend/i_search_backend.h
    search_backend/library_search_backend.h
    search_backend/library_search_backend.cpp
    search_backend/in_memory_search_backend.h
    search_backend/in_memory_search_backend.cpp

    shortcuts_controller.cpp
    shortcuts_controller.h

    status_bar_controller.cpp
    status_bar_controller.h

    plugin_controller/plugin_controller.cpp
    plugin_controller/plugin_controller.h
)

target_link_libraries(wusic_controller
    PUBLIC
        Qt6::Core
        Qt6::Widgets
        Qt6::Multimedia
        wusic_model
        wusic_core
        wusic_headers
)