add_library(wusic_service STATIC
    playback_service.cpp
    playback_service.h

    playback_restore_service.cpp
    playback_restore_service.h

    library_interaction_service.cpp
    library_interaction_service.h
    tag_writeback_service.cpp
    tag_writeback_service.h

    theme_service.cpp
    theme_service.h

    notification_service/i_notification_display.h
    notification_service/notification_service.cpp
    notification_service/notification_service.h
)

target_link_libraries(wusic_service
    PUBLIC
        Qt6::Core
        Qt6::Widgets
        wusic_core
        wusic_headers
        lrc_parser
)