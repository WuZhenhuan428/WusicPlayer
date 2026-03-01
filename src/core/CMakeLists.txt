add_library(wusic_core STATIC
    # add existing code here
    types.h
    ConfigManager/ConfigManager.h
    ConfigManager/ConfigManager.cpp
    utils/AudioUtils.h
)

target_link_libraries(wusic_core
    PUBLIC
        Qt6::Core
        wusic_headers
)