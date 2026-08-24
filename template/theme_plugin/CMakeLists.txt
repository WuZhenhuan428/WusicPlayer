cmake_minimum_required(VERSION 3.16)
project(wusic_theme_mytheme LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON) # 头文件含 Q_OBJECT, 必须有 moc

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

# WusicPlayer 项目根目录(按实际位置调整)
set(WUSIC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../WusicPlayer")

add_library(wusic_theme_mytheme MODULE
    my_theme_plugin.cpp
    my_theme_plugin.h
    my_theme_plugin.json
)

target_include_directories(wusic_theme_mytheme PRIVATE
    "${WUSIC_ROOT}/include" # 插件接口头(plugin/i_theme_plugin.h 等)
    "${WUSIC_ROOT}/src"     # 项目源码头(theme_palette.h 等)
)

target_link_libraries(wusic_theme_mytheme PRIVATE Qt6::Core Qt6::Widgets)

# 去掉 lib 前缀, 生成 my_theme_plugin.so
set_target_properties(wusic_theme_mytheme PROPERTIES PREFIX "")

# 构建后复制到应用 plugins/ 目录(可按需启用)
# set_target_properties(wusic_theme_mytheme PROPERTIES
#     LIBRARY_OUTPUT_DIRECTORY "${WUSIC_ROOT}/build/debug/src/plugins")
