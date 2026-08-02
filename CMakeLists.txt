cmake_minimum_required(VERSION 3.21)
project(WusicPlayer LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(WUSIC_BUILD_TESTS "Build unit tests" ON)
option(WUSIC_ENABLE_GPROF "Enable gprof instrumentation (-pg)" OFF)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Werror -Wall -Wextra)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(/W4)
endif ()

if (WUSIC_ENABLE_GPROF)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-pg)
        add_link_options(-pg)
        message(STATUS "gprof instrumentation enabled (-pg)")
    else ()
        message(WARNING "WUSIC_ENABLE_GPROF is ON, but compiler ${CMAKE_CXX_COMPILER_ID} may not support -pg")
    endif ()
endif ()

# Custom find modules (cmake/FindFFmpeg.cmake etc.)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Multimedia Network Svg Sql)
find_package(FFmpeg REQUIRED)       # cmake/FindFFmpeg.cmake
find_package(Taglib REQUIRED)       # CMake built-in module (uses pkg-config internally)
# Normalize target name — some distros' CMake configs don't create a target at all
if(NOT TARGET Taglib::Taglib)
    if(TARGET Taglib::taglib)
        add_library(Taglib::Taglib ALIAS Taglib::taglib)
    elseif(TARGET taglib)
        add_library(Taglib::Taglib ALIAS taglib)
    else()
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(TAGLIB_PC REQUIRED IMPORTED_TARGET taglib)
        add_library(Taglib::Taglib ALIAS PkgConfig::TAGLIB_PC)
        message(STATUS "Taglib: using pkg-config (no CMake target provided)")
    endif()
endif()
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)

# magic_enum: header-only, vendored as git submodule
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/magic_enum/CMakeLists.txt")
    add_subdirectory(thirdparty/magic_enum)
else()
    message(FATAL_ERROR
        "thirdparty/magic_enum/ not found.\n"
        "Run:  git submodule update --init --recursive\n"
        "Or:   git submodule add https://github.com/Neargye/magic_enum.git thirdparty/magic_enum"
    )
endif()

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
