if(BUILD_GENKEY)
    # genkey is a rather small application
    set(genkey_sources
        engine/genkey.cpp
        shared/crypto.cpp
    )

    add_blue_nebula_executable(genkey${BIN_SUFFIX} ${genkey_sources})

    target_link_libraries(genkey${BIN_SUFFIX} ZLIB::ZLIB)

    # like for the server, we also have to define STANDALONE here to avoid dependencies on SDL2
    set_target_properties(genkey${BIN_SUFFIX} PROPERTIES
        COMPILE_FLAGS "-DSTANDALONE"
    )
endif()
