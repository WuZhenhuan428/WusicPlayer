cmake_minimum_required(VERSION 3.19)
project(WusicPlayer LANGUAGES CXX)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Widgets)
find_package(Qt6 REQUIRED COMPONENTS Multimedia)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TAGLIB REQUIRED taglib)

qt_standard_project_setup()

qt_add_executable(WusicPlayer
    WIN32 MACOSX_BUNDLE
    main.cpp
    MainWindow.h MainWindow.cpp

    # include
    include/audio.h

    # src/player
    src/player/player.h
    src/player/player.cpp

    # src/playlist
    src/playlist/playlist_definitions.h
    src/playlist/playlist.h
    src/playlist/playlist.cpp
    src/playlist/playlist_context.h
    src/playlist/playlist_context.cpp
    src/playlist/playlist_manager.h
    src/playlist/playlist_manager.cpp
    src/playlist/playlist_repo.h
    src/playlist/playlist_repo.cpp
    src/playlist/playlist_view_model.h
    src/playlist/playlist_view_model.cpp
    src/playlist/playlist_layout.cpp
    src/playlist/playlist_layout.h
    src/playlist/playlist_widgets.cpp
    src/playlist/playlist_widgets.h

    # custom widgets
    src/WControlBar/WControlBar.cpp
    src/WControlBar/WControlBar.h
    src/WControlBar/wtimeprogress.cpp
    src/WControlBar/wtimeprogress.h

    # config manager
    src/ConfigManager/ConfigManager.cpp
    src/ConfigManager/ConfigManager.h

    # lyrics panel
    src/WLyricsPanel/WLyricsPanel.cpp
    src/WLyricsPanel/WLyricsPanel.h
    src/WLyricsPanel/WLyricsModel.cpp
    src/WLyricsPanel/WLyricsModel.h
)

target_include_directories(WusicPlayer
    PRIVATE
        src/playlist
        include
        ${TAGLIB_INCLUDE_DIRS}
)

add_subdirectory(thirdparty/lrc-parser)

target_link_libraries(WusicPlayer
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Multimedia
        ${TAGLIB_LIBRARIES}
        lrc_parser
)



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
