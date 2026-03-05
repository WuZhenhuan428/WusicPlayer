add_executable(tb_playlist
    playlist/tb_playlist.cpp
)

target_link_libraries(tb_playlist
    PRIVATE
        wusic_model
        wusic_core
        wusic_headers
)

add_executable(AudioScanner
    AudioScanner/AudioScanner.cpp
)

add_test(NAME tb_playlist COMMAND tb_playlist)
add_test(NAME AudioScanner COMMAND AudioScanner)
