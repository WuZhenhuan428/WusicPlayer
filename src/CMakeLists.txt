add_library(wusic_headers INTERFACE)
target_include_directories(wusic_headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(core)
add_subdirectory(model)
add_subdirectory(view)
add_subdirectory(controller)
add_subdirectory(static)

qt_add_executable(WusicPlayer
    main.cpp
)

wusic_add_static_resources(WusicPlayer)

target_link_libraries(WusicPlayer
    PRIVATE
        wusic_controller
        wusic_view
        wusic_model
        wusic_core
        wusic_headers
)