# genkey is a rather small application
file(GLOB genkey_sources
    engine/genkey.cpp
    shared/crypto.cpp
)

# TODO: find out how to build genkey properly on macOS
if(NOT APPLE)
    add_redeclipse_executable(genkey${BIN_SUFFIX} ${genkey_sources})

    # like for the server, we also have to define STANDALONE here to avoid dependencies on SDL2
    set_target_properties(genkey${BIN_SUFFIX} PROPERTIES
        COMPILE_FLAGS "-DSTANDALONE"
    )
endif()
