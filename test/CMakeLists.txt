add_executable(test_playlist
    playlist/tb_playlist.cpp
)

target_link_libraries(test_playlist
    PRIVATE
        wusic_model
        wusic_core
        wusic_headers
)

add_test(NAME tb_playlist COMMAND tb_playlist)