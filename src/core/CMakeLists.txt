add_library(wusic_core STATIC
    # add existing code here
    types.h
    utils/AudioUtils.h
    ConfigManager/ConfigManager.h
    ConfigManager/ConfigManager.cpp

    ConfigManager/IConfigSection.hpp
    ConfigManager/LibraryViewSection.hpp
    ConfigManager/PlaybackConfigSection.hpp
    ConfigManager/SearchPanelSection.hpp
    ConfigManager/WindowConfigSection.hpp
    ConfigManager/DesktopLyricsSection.hpp
)

target_link_libraries(wusic_core
    PUBLIC
        Qt6::Core
        wusic_headers
        magic_enum::magic_enum
)