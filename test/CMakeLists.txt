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

# ======== utils_audio ========

add_executable(utils_audio
    utils_audio/main.cpp
)

target_link_libraries(utils_audio
    PRIVATE
        wusic_core
)

add_test(NAME utils_audio COMMAND utils_audio)
