# wusic_headers: 统一的 include 路径
#  - src/            : 内部头(与 cpp Side-By-Side)
#  - ../include      : 对外使用的公共头
#  - ../thirdparty/headeronly : 手动管理的 header-only 第三方库(如 miniaudio.h)
add_library(wusic_headers INTERFACE)
target_include_directories(wusic_headers INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/headeronly
)

add_subdirectory(core)
add_subdirectory(model)
add_subdirectory(view)
add_subdirectory(controller)
add_subdirectory(static)
add_subdirectory(service)

qt_add_executable(WusicPlayer
    main.cpp
    app_controller.cpp
    app_controller.h
    panel_coordinator.cpp
    panel_coordinator.h
)

wusic_add_static_resources(WusicPlayer)

target_link_libraries(WusicPlayer
    PRIVATE
        wusic_controller
        wusic_view
        wusic_model
        wusic_core
        wusic_headers
        wusic_service
)

# 外部插件经 dlopen 加载, 需要解析宿主可执行文件中的接口符号
# (如 IBasicPlugin::staticMetaObject, 由静态库 wusic_core 提供);
# 可执行文件默认不导出符号, 这里开启 --export-dynamic。
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_options(WusicPlayer PRIVATE "-rdynamic")
endif()