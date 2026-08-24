add_library(wusic_core STATIC
    types.h
    hsv_types.h
    search_types.h
    event_bus.h
    event_bus.cpp
    
    logger/logger.h
    logger/logger_manager.cpp
    logger/logger_manager.h
    logger/log_level.h
    logger/log_record.h
    logger/log_sink_console.cpp
    logger/log_sink_console.h
    logger/log_sink_gui.cpp
    logger/log_sink_gui.h
    logger/log_sink.h

    utils/audio.hpp
    utils/path.hpp
    utils/font.hpp

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
    theme/builtin/builtin_theme_plugin.h
    theme/builtin/builtin_theme_plugin.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/plugin/i_theme_plugin.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/plugin/eq_types.h

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

    plugin_manager/plugin_manager.cpp
    plugin_manager/plugin_manager.h
    plugin_manager/i_basic_plugin.cpp
    plugin_manager/i_eq_plugin.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/plugin/i_basic_plugin.h

    eq/builtin/builtin_eq_plugin.h
    eq/builtin/builtin_eq_plugin.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/plugin/i_eq_plugin.h
)

# 插件接口(IBasicPlugin 等)的 moc 必须可被外部插件 .so 链接, 故需要 PIC
set_target_properties(wusic_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

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
