add_library(wusic_controller STATIC
    PlaybackController.h
    PlaybackController.cpp

    PlaylistController.h
    PlaylistController.cpp

    search_backend/i_search_backend.h
    search_backend/i_search_backend.cpp
    search_backend/in_memory_search_backend.h
    search_backend/in_memory_search_backend.cpp

    shortcuts_controller.cpp
    shortcuts_controller.h
)

target_link_libraries(wusic_controller
    PUBLIC
        Qt6::Core
        Qt6::Widgets
        wusic_model
        wusic_core
        wusic_headers
)