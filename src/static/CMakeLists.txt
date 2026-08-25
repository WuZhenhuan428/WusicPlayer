function(wusic_add_static_resources target_name)
    if (NOT TARGET ${target_name})
        message(FATAL_ERROR "Target '${target_name}' does not exist.")
    endif()

    set(_static_root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}")

    set(_files
        "${_static_root}/images/test_cover_chirno.png"

        "${_static_root}/icons/main.ico"
	"${_static_root}/icons/main_64_64.ico"
        "${_static_root}/icons/exit.svg"

        "${_static_root}/icons/light/device.svg"
        "${_static_root}/icons/light/in_order.svg"
        "${_static_root}/icons/light/loop.svg"
        "${_static_root}/icons/light/next.svg"
        "${_static_root}/icons/light/out_of_order_group.svg"
        "${_static_root}/icons/light/out_of_order_track.svg"
        "${_static_root}/icons/light/pause.svg"
        "${_static_root}/icons/light/play.svg"
        "${_static_root}/icons/light/prev.svg"
        "${_static_root}/icons/light/shuffle.svg"
        "${_static_root}/icons/light/stop.svg"
        "${_static_root}/icons/light/volume_0.svg"
        "${_static_root}/icons/light/volume_1.svg"
        "${_static_root}/icons/light/volume_2.svg"
        "${_static_root}/icons/light/volume_3.svg"
        "${_static_root}/icons/light/volume_x.svg"
        "${_static_root}/icons/light/maximum.svg"
        "${_static_root}/icons/light/minimum.svg"

        "${_static_root}/icons/dark/device.svg"
        "${_static_root}/icons/dark/in_order.svg"
        "${_static_root}/icons/dark/loop.svg"
        "${_static_root}/icons/dark/next.svg"
        "${_static_root}/icons/dark/out_of_order_group.svg"
        "${_static_root}/icons/dark/out_of_order_track.svg"
        "${_static_root}/icons/dark/pause.svg"
        "${_static_root}/icons/dark/play.svg"
        "${_static_root}/icons/dark/prev.svg"
        "${_static_root}/icons/dark/shuffle.svg"
        "${_static_root}/icons/dark/stop.svg"
        "${_static_root}/icons/dark/volume_0.svg"
        "${_static_root}/icons/dark/volume_1.svg"
        "${_static_root}/icons/dark/volume_2.svg"
        "${_static_root}/icons/dark/volume_3.svg"
        "${_static_root}/icons/dark/volume_x.svg"
        "${_static_root}/icons/dark/maximum.svg"
        "${_static_root}/icons/dark/minimum.svg"
    )

    qt_add_resources(${target_name} "wusic_assets"
        PREFIX "/"
        BASE "${_static_root}"
        FILES ${_files}
    )
endfunction()
