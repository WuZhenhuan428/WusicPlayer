# ======== tb_playlist ========

add_executable(tb_playlist
    playlist/tb_playlist.cpp
)

target_link_libraries(tb_playlist
    PRIVATE
        wusic_model
        wusic_core
        wusic_headers
)

add_test(NAME tb_playlist COMMAND tb_playlist)

# ======== tb_library ========

add_executable(tb_library
    library/tb_library.cpp
)

target_link_libraries(tb_library
    PRIVATE
        wusic_model
        wusic_controller
        wusic_core
        wusic_headers
)

add_test(NAME tb_library COMMAND tb_library)

# ======== tb_library_browse ========

add_executable(tb_library_browse
    library/tb_library_browse.cpp
)

target_link_libraries(tb_library_browse
    PRIVATE
        wusic_model
        wusic_core
        wusic_headers
)

add_test(NAME tb_library_browse COMMAND tb_library_browse)

# ======== tb_search_backend ========

add_executable(tb_search_backend
    search/tb_search_backend.cpp
)

target_link_libraries(tb_search_backend
    PRIVATE
        wusic_controller
        wusic_model
        wusic_core
        wusic_headers
)

add_test(NAME tb_search_backend COMMAND tb_search_backend)

# ======== tb_playback_queue ========

add_executable(tb_playback_queue
    playback_queue/tb_playback_queue.cpp
)

target_link_libraries(tb_playback_queue
    PRIVATE
        wusic_model
        wusic_core
        wusic_headers
)

add_test(NAME tb_playback_queue COMMAND tb_playback_queue)

# ======== utils_audio ========

add_executable(utils_audio
    utils_audio/main.cpp
)

target_link_libraries(utils_audio
    PRIVATE
        wusic_core
)

add_test(NAME utils_audio COMMAND utils_audio)
