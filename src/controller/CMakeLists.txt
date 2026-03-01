add_library(wusic_controller STATIC
    PlaybackController.h
    PlaybackController.cpp
    PlaylistController.h
    PlaylistController.cpp
    # ...existing code...
)

target_link_libraries(wusic_controller
    PUBLIC
        Qt6::Core
        Qt6::Widgets
        wusic_model
        wusic_core
        wusic_headers
)