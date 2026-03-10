add_library(wusic_core STATIC
    # add existing code here
    types.h
    utils/AudioUtils.h
    ConfigManager/ConfigManager.h
    ConfigManager/ConfigManager.cpp

)

target_link_libraries(wusic_core
    PUBLIC
        Qt6::Core
        wusic_headers
        magic_enum::magic_enum
)