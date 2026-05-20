#[=======================================================================[.rst:
FindFFmpeg
----------

Find FFmpeg libraries (avcodec avformat avutil avfilter).

- On Linux / MSYS2 / Homebrew: uses ``pkg-config`` (zero config needed).
- On Windows (MSVC / standalone MinGW): set ``FFMPEG_ROOT`` to the
  folder containing ``include/`` and ``lib/``.

Imported Targets
^^^^^^^^^^^^^^^^

``FFMPEG::FFMPEG``
  Umbrella target covering all requested FFmpeg libraries.

#]=======================================================================]

# ---- Path 1: pkg-config (Linux, MSYS2, Homebrew) ----
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(_FFMPEG_PC IMPORTED_TARGET
        libavcodec libavformat libavutil libavfilter)
endif()

if(_FFMPEG_PC_FOUND)
    if(NOT TARGET FFMPEG::FFMPEG)
        add_library(FFMPEG::FFMPEG INTERFACE IMPORTED)
        target_link_libraries(FFMPEG::FFMPEG INTERFACE PkgConfig::_FFMPEG_PC)
    endif()
    set(FFmpeg_FOUND TRUE)
    return()
endif()

# ---- Path 2: manual search (Windows pre-built FFmpeg) ----
include(FindPackageHandleStandardArgs)

find_path(FFmpeg_INCLUDE_DIR
    NAMES libavcodec/avcodec.h
    PATHS ${FFMPEG_ROOT} $ENV{FFMPEG_ROOT}
    PATH_SUFFIXES include ffmpeg
)

set(_FFmpeg_LIBS)
set(_FFmpeg_REQUIRED_VARS FFmpeg_INCLUDE_DIR)

foreach(_lib IN ITEMS avcodec avformat avutil avfilter)
    find_library(FFmpeg_${_lib}_LIBRARY
        NAMES ${_lib}
        PATHS ${FFMPEG_ROOT} $ENV{FFMPEG_ROOT}
        PATH_SUFFIXES lib
    )
    list(APPEND _FFmpeg_LIBS ${FFmpeg_${_lib}_LIBRARY})
    list(APPEND _FFmpeg_REQUIRED_VARS FFmpeg_${_lib}_LIBRARY)
    mark_as_advanced(FFmpeg_${_lib}_LIBRARY)
endforeach()

find_package_handle_standard_args(FFmpeg
    REQUIRED_VARS ${_FFmpeg_REQUIRED_VARS}
)

if(FFmpeg_FOUND AND NOT TARGET FFMPEG::FFMPEG)
    add_library(FFMPEG::FFMPEG INTERFACE IMPORTED)
    set_target_properties(FFMPEG::FFMPEG PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${FFmpeg_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "${_FFmpeg_LIBS}"
    )
endif()

mark_as_advanced(FFmpeg_INCLUDE_DIR)

