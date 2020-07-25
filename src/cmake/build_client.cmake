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
        engine/movie.cpp
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
        engine/animmodel.h
        engine/bih.h
        engine/depthfx.h
        engine/engine.h
        engine/explosion.h
        engine/iqm.h
        engine/irc.h
        engine/lensflare.h
        engine/lightmap.h
        engine/lightning.h
        engine/md2.h
        engine/md3.h
        engine/md5.h
        engine/model.h
        engine/mpr.h
        engine/obj.h
        engine/octa.h
        engine/ragdoll.h
        engine/rendertarget.h
        engine/scale.h
        engine/skelmodel.h
        engine/smd.h
        engine/sound.h
        engine/textedit.h
        engine/texture.h
        engine/version.h
        engine/vertmodel.h
        engine/world.h
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
        game/ai.h
        game/aiman.h
        game/auth.h
        game/bomber.h
        game/bombermode.h
        game/capture.h
        game/capturemode.h
        game/compass.h
        game/defend.h
        game/defendmode.h
        game/duelmut.h
        game/game.h
        game/gamemode.h
        game/player.h
        game/playerdef.h
        game/teamdef.h
        game/vars.h
        game/weapdef.h
        game/weapons.h
        shared/crypto.cpp
        shared/geom.cpp
        shared/glemu.cpp
        shared/stream.cpp
        shared/tools.cpp
        shared/zip.cpp
        shared/command.h
        shared/cube.h
        shared/ents.h
        shared/geom.h
        shared/glemu.h
        shared/glexts.h
        shared/iengine.h
        shared/igame.h
        shared/tools.h
    )

    # dependencies are imported globally in src/CMakeLists.txt
    # we can just list them by name here
    set(client_deps ZLIB::ZLIB SDL2::Main SDL2::Image SDL2::Mixer OpenGL::GL enet)

    # platform specific code
    if(APPLE)
        # build OS X specific Objective-C code
        set(mac_client_sources
            xcode/SDLMain.h
            xcode/SDLMain.m
            xcode/macutils.mm
            xcode/ConsoleView.h
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
