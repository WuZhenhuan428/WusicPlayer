add_library(wusic_service STATIC
    playback_service.cpp
    playback_service.h

    playback_restore_service.cpp
    playback_restore_service.h

    # TODO
    # library_interaction_service.cpp
    # library_interaction_service.h
    # tag_write_back.cpp
    # tag_write_back.h
    # setting.cpp
    # setting.h
    # search_service.cpp
    # search_service.h
)

target_link_libraries(wusic_service
    PUBLIC
        Qt6::Core
        Qt6::Widgets
        wusic_core
        wusic_headers
        lrc_parser
)