# install bundled binaries
if(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
    if(MINGW_PLATFORM)
        message(STATUS "Building with MinGW cross toolchain, prefix: ${MINGW_PREFIX}")
        set(WINDOWS_LIBRARY_PREFIX ${MINGW_PREFIX})
    else()
        # we only support x86_64/i686 architectures, so we can "guess" the prefixes
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(WINDOWS_LIBRARY_PREFIX x86_64-w64-mingw32)
        elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
            set(WINDOWS_LIBRARY_PREFIX i686-w64-mingw32)
        else()
            message(FATAL_ERROR "Unknown platform, please re-run with -DWINDOWS_LIBRARY_PREFIX=...")
        endif()
        message(STATUS "Building for Windows without cross toolchain, guessing prefix ${WINDOWS_LIBRARY_PREFIX}")
    endif()

    message(STATUS "Installing Windows binaries into bin directory")
    file(GLOB BUNDLED_LIBS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/bundled-libs/${WINDOWS_LIBRARY_PREFIX}/lib/*)
    install(
        FILES ${BUNDLED_LIBS_FILES}
        DESTINATION bin
    )
endif()
