if(BUILD_GENKEY)
    # genkey is a rather small application
    set(genkey_sources
        # list generated using
        # scripts/trace-headers.py -DSTANDALONE <all .cpp files listed below>
        engine/genkey.cpp
        shared/command.h
        shared/crypto.cpp
        shared/cube.h
        shared/ents.h
        shared/geom.h
        shared/glemu.h
        shared/glexts.h
        shared/iengine.h
        shared/igame.h
        shared/tools.h

        engine/genkey.cpp
        shared/crypto.cpp
    )

    add_blue_nebula_executable(genkey ${genkey_sources})

    target_link_libraries(genkey ZLIB::ZLIB)

    # like for the server, we also have to define STANDALONE here to avoid dependencies on SDL2
    set_target_properties(genkey PROPERTIES
        COMPILE_FLAGS "-DSTANDALONE"
    )
endif()
