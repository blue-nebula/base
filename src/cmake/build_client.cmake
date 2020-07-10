if(BUILD_CLIENT)
    # the client depends on almost all the source files
    set(client_sources
        engine/bih.cpp
        engine/blend.cpp
        engine/blob.cpp
        engine/client.cpp
        engine/command.cpp
        engine/console.cpp
        engine/decal.cpp
        engine/dynlight.cpp
        engine/glare.cpp
        engine/grass.cpp
        engine/irc.cpp
        engine/lightmap.cpp
        engine/main.cpp
        engine/master.cpp
        engine/material.cpp
        engine/menus.cpp
        engine/normal.cpp
        engine/octa.cpp
        engine/octaedit.cpp
        engine/octarender.cpp
        engine/physics.cpp
        engine/pvs.cpp
        engine/rendergl.cpp
        engine/rendermodel.cpp
        engine/renderparticles.cpp
        engine/rendersky.cpp
        engine/rendertext.cpp
        engine/renderva.cpp
        engine/server.cpp
        engine/serverbrowser.cpp
        engine/shader.cpp
        engine/shadowmap.cpp
        engine/sound.cpp
        engine/texture.cpp
        engine/ui.cpp
        engine/water.cpp
        engine/world.cpp
        engine/worldio.cpp
        game/ai.cpp
        game/bomber.cpp
        game/capture.cpp
        game/client.cpp
        game/defend.cpp
        game/entities.cpp
        game/game.cpp
        game/hud.cpp
        game/physics.cpp
        game/projs.cpp
        game/scoreboard.cpp
        game/server.cpp
        game/waypoint.cpp
        game/weapons.cpp
        shared/crypto.cpp
        shared/geom.cpp
        shared/glemu.cpp
        shared/stream.cpp
        shared/tools.cpp
        shared/zip.cpp
    )

    # dependencies are imported globally in src/CMakeLists.txt
    # we can just list them by name here
    set(client_deps ZLIB::ZLIB SDL2::Main SDL2::Image SDL2::Mixer OpenGL::GL enet)

    # platform specific code
    if(APPLE)
        # build OS X specific Objective-C code
        set(mac_client_sources
            xcode/SDLMain.m
            xcode/macutils.mm
            xcode/ConsoleView.m
        )
        list(APPEND client_sources ${mac_client_sources})
    elseif(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(x11 x11 REQUIRED IMPORTED_TARGET)
        list(APPEND client_deps PkgConfig::x11 rt)
    endif()

    # finally, add the executable build configuration
    add_blue_nebula_executable(${APPNAME}${BIN_SUFFIX} ${client_sources})

    # CMake will also configure include dirs etc. for all targets linked against with target_link_libraries
    target_link_libraries(${APPNAME}${BIN_SUFFIX} ${client_deps})

    # make sure .rc file is added
    add_windows_rc_file(${APPNAME}${BIN_SUFFIX})
endif()
