cmake_minimum_required(VERSION 3.24)

if (WUSIC_USE_ASAN)
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        set(sanitize_flags
            "-fsanitize=address"
            "-fno-omit-frame-pointer"
            "-g"
        )
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        set(sanitize_flags
            "/fsanitize=address"
        )
      add_compile_options("/Zi")
    endif()

    add_compile_options(${sanitize_flags})
    add_link_options(${sanitize_flags})
endif()

