cmake_minimum_required(VERSION 3.21)
project(WusicPlayer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(WUSIC_BUILD_TESTS "Build unit tests" ON)

add_compile_options(-Wall -Wextra)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Multimedia)
find_package(magic_enum CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(TAGLIB REQUIRED IMPORTED_TARGET taglib)

qt_standard_project_setup()

add_subdirectory(thirdparty/lrc-parser)
add_subdirectory(src)

if (WUSIC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()


include(GNUInstallDirs)

install(TARGETS WusicPlayer
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET WusicPlayer
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
