add_library(wusic_core STATIC
    types.h
    hsv_types.h
    search_types.h
    player_types.h
    
    utils/audio.hpp
    utils/path_utils.hpp

    ConfigManager/ConfigManager.h
    ConfigManager/ConfigManager.cpp
    ConfigManager/IConfigurable.h

    theme/ThemePalette.h
    theme/WusicProxyStyle.h
    theme/WusicProxyStyle.cpp
    theme/ThemeManager.h
    theme/ThemeManager.cpp
    theme/builtin/WusicDarkPalette.h
    theme/builtin/WusicLightPalette.h
    theme/plugin/IThemePlugin.h

    LyricsFetcher/lyrics_manager.h
    LyricsFetcher/lyrics_manager.cpp

    LyricsFetcher/netease_qt6.h
    LyricsFetcher/netease_qt6.cpp

    player/config.h
    player/decoder.cpp
    player/decoder.h
    player/device.cpp
    player/device.h
    player/miniaudio.h
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
)