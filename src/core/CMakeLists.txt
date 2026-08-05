add_library(wusic_core STATIC
    types.h
    hsv_types.h
    search_types.h
    player_types.h
    
    utils/audio.hpp
    utils/path.hpp

    config_manager/config_manager.h
    config_manager/config_manager.cpp
    config_manager/i_configurable.h

    theme/theme_palette.h
    theme/wusic_proxy_style.h
    theme/wusic_proxy_style.cpp
    theme/theme_manager.h
    theme/theme_manager.cpp
    theme/builtin/wusic_dark_palette.h
    theme/builtin/wusic_light_palette.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/plugin/i_theme_plugin.h

    lyrics_fetcher/lyrics_manager.h
    lyrics_fetcher/lyrics_manager.cpp

    lyrics_fetcher/netease_qt6.h
    lyrics_fetcher/netease_qt6.cpp

    player/config.h
    player/decoder.cpp
    player/decoder.h
    player/device.cpp
    player/device.h
    player/miniaudio_impl.c
    player/player.cpp
    player/player_engine.cpp
    player/player_engine.h
    player/player.h
    player/ring_buffer.hpp
)

target_link_libraries(wusic_core
    PUBLIC
        Qt6::Core
        Qt6::Widgets
        Qt6::Multimedia
        Qt6::Network
        OpenSSL::Crypto
        ZLIB::ZLIB
        wusic_headers
        magic_enum::magic_enum
        FFMPEG::FFMPEG
        Threads::Threads
        $<$<CXX_COMPILER_ID:GNU>:atomic>
        stdc++exp
)
