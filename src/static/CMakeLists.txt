function(wusic_add_static_resources target_name)
    if (NOT TARGET ${target_name})
        message(FATAL_ERROR "Target '${target_name}' does not exist.")
    endif()

    set(_static_root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}")

    set(_files
        "${_static_root}/images/test_cover_chirno.png"
        # "${_static_root}/icons/app.svg"
    )

    qt_add_resources(${target_name} "wusic_assets"
        PREFIX "/"
        BASE "${_static_root}"
        FILES ${_files}
    )
endfunction()