cmake_minimum_required(VERSION 3.21)
project(WusicPlayer LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(WUSIC_BUILD_TESTS "Build unit tests" ON)
option(WUSIC_USE_LOCAL_QT "Prefer local Qt installation over system Qt" ON)
option(WUSIC_ENABLE_GPROF "Enable gprof instrumentation (-pg)" OFF)

set(WUSIC_LOCAL_QT_PATH "$ENV{HOME}/Qt/6.10.2/gcc_64" CACHE PATH "Local Qt installation prefix")

if (WUSIC_USE_LOCAL_QT)
    if (EXISTS "${WUSIC_LOCAL_QT_PATH}/lib/cmake/Qt6/Qt6Config.cmake")
        list(PREPEND CMAKE_PREFIX_PATH "${WUSIC_LOCAL_QT_PATH}")
        message(STATUS "Using local Qt from: ${WUSIC_LOCAL_QT_PATH}")
    else ()
        message(WARNING "WUSIC_USE_LOCAL_QT is ON but Qt6Config.cmake was not found in ${WUSIC_LOCAL_QT_PATH}")
    endif ()
endif ()

add_compile_options(-Wall -Wextra)

if (WUSIC_ENABLE_GPROF)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-pg)
        add_link_options(-pg)
        message(STATUS "gprof instrumentation enabled (-pg)")
    else ()
        message(WARNING "WUSIC_ENABLE_GPROF is ON, but compiler ${CMAKE_CXX_COMPILER_ID} may not support -pg")
    endif ()
endif ()

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
