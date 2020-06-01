if(BUILD_CLIENT)
    # the client depends on almost all the source files
    file(GLOB client_sources engine/*.cpp game/*.cpp shared/*.cpp)

    # the client does not need genkey.cpp
    # to avoid warnings about duplicate main()s, it has to be removed from their source lists
    file(GLOB genkey_cpp_path engine/genkey.cpp)
    list(REMOVE_ITEM client_sources ${genkey_cpp_path})

    # dependencies are imported globally in src/CMakeLists.txt
    # we can just list them by name here
    set(client_deps ZLIB::ZLIB SDL2::Main SDL2::Image SDL2::Mixer OpenGL::GL enet)

    # platform specific code
    if(APPLE)
        # build OS X specific Objective-C code
        file(GLOB mac_client_sources xcode/SDLmain.m xcode/macutils.mm xcode/ConsoleView.m)

        # on macOS natively, we don't seem to require that file (any more?)
        # osxcross however appears to need it
        if(OSXCROSS_HOST)
            message(STATUS "osxcross build, building main.m for client")
            list(APPEND mac_client_sources xcode/main.m)
        endif()
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
