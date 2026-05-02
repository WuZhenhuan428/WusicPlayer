function(wusic_add_static_resources target_name)
    if (NOT TARGET ${target_name})
        message(FATAL_ERROR "Target '${target_name}' does not exist.")
    endif()

    set(_static_root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}")

    set(_files
        "${_static_root}/images/test_cover_chirno.png"

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

    )

    qt_add_resources(${target_name} "wusic_assets"
        PREFIX "/"
        BASE "${_static_root}"
        FILES ${_files}
    )
endfunction()
