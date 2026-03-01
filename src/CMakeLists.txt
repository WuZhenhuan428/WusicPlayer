add_library(wusic_headers INTERFACE)
target_include_directories(wusic_headers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(core)
add_subdirectory(model)
add_subdirectory(view)
add_subdirectory(controller)

qt_add_executable(WusicPlayer
    main.cpp
)

target_link_libraries(WusicPlayer
    PRIVATE
        wusic_view
        wusic_controller
        wusic_model
        wusic_core
        wusic_headers
)