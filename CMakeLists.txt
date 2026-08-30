cmake_minimum_required(VERSION 3.24)
project(WusicPlayer LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 23)
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

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets Network Svg Sql)
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

# ---- 多语言(i18n): lupdate 提取 / lrelease 编译 ----
# 跨平台策略:
#   1) 优先使用 Qt LinguistTools 的导入目标 Qt6::lupdate / Qt6::lrelease,
#      由 Qt CMake 包自动定位工具(Windows 官方 Qt / MSYS2、Linux、macOS 均可用)
#   2) 回退 find_program, 兼容只装了可执行文件、没有 CMake 包的发行版
find_package(Qt6 COMPONENTS LinguistTools QUIET)

if(TARGET Qt6::lupdate)
    set(QT_LUPDATE_EXECUTABLE Qt6::lupdate)
else()
    # 优先 Qt6 自带的翻译工具(避免误用系统 Qt5 wrapper)
    find_program(QT_LUPDATE_EXECUTABLE
        NAMES lupdate-qt6 lupdate
        HINTS "${QT6_INSTALL_BINS}" "${CMAKE_INSTALL_PREFIX}/lib/qt6/bin" /usr/lib/qt6/bin
        DOC "Path to the Qt6 lupdate tool")
endif()

if(TARGET Qt6::lrelease)
    set(QT_LRELEASE_EXECUTABLE Qt6::lrelease)
else()
    find_program(QT_LRELEASE_EXECUTABLE
        NAMES lrelease-qt6 lrelease
        HINTS "${QT6_INSTALL_BINS}" "${CMAKE_INSTALL_PREFIX}/lib/qt6/bin" /usr/lib/qt6/bin
        DOC "Path to the Qt6 lrelease tool")
endif()

if(QT_LUPDATE_EXECUTABLE AND QT_LRELEASE_EXECUTABLE)
    set(WUSIC_TRANSLATIONS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/translations")
    set(WUSIC_TS_FILES
        "${WUSIC_TRANSLATIONS_DIR}/wusicplayer_zh_CN.ts"
    )
    set(WUSIC_QM_DIR "${CMAKE_CURRENT_BINARY_DIR}/translations")

    # 更新 .ts(手动: cmake --build build --target translations_update)
    add_custom_target(translations_update
        COMMAND ${QT_LUPDATE_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/src
                -ts ${WUSIC_TS_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Updating translation source files (.ts)"
    )

    # 编译 .qm(构建时自动执行)
    add_custom_command(
        OUTPUT "${WUSIC_QM_DIR}/wusicplayer_zh_CN.qm"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${WUSIC_QM_DIR}"
        COMMAND ${QT_LRELEASE_EXECUTABLE} ${WUSIC_TS_FILES}
                -qm "${WUSIC_QM_DIR}/wusicplayer_zh_CN.qm"
        DEPENDS ${WUSIC_TS_FILES}
        COMMENT "Compiling translations to .qm"
    )
    add_custom_target(translations ALL
        DEPENDS "${WUSIC_QM_DIR}/wusicplayer_zh_CN.qm"
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "$<TARGET_FILE_DIR:WusicPlayer>/translations"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${WUSIC_QM_DIR}/wusicplayer_zh_CN.qm"
                "$<TARGET_FILE_DIR:WusicPlayer>/translations/"
        COMMENT "Deploying .qm next to the executable"
    )
    message(STATUS "i18n: lupdate/lrelease enabled")
else()
    message(WARNING "lupdate/lrelease not found; translations disabled")
endif()

if (WUSIC_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

if(WIN32)
   set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE ON)
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
