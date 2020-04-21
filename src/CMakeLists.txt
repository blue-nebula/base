# 3.6+ needed for pkg-config's IMPORTED targets, as well as IMPORTED targets for zlib
# 3.8+ needed for OpenGL's IMPORTED targets
cmake_minimum_required(VERSION 3.8)

# set compile options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -fomit-frame-pointer -ffast-math -Wall -fsigned-char -fno-exceptions -fno-rtti")

# we use find_package instead of pkg_check_modules etc. so we do not depend on .pc files for libraries used across all platforms
# this allows for easier cross-compiling with MinGW, as we can use the precompiled libraries
find_package(ZLIB REQUIRED)

# for SDL2* we need to ship custom configs
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_mixer REQUIRED)

# OpenGL may be imported with pkg-config on Unix, but that doesn't work on macOS
set(OpenGL_GL_PREFERENCE LEGACY)
find_package(OpenGL REQUIRED)

# platform specific code
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(BIN_SUFFIX "_linux")
elseif(APPLE)
    set(BIN_SUFFIX "_osx")
elseif(MINGW)
    set(BIN_SUFFIX "_windows")
else()
    message(WARNING "Unknown build platform")
    set(BIN_SUFFIX "_unknown")
endif()

# make sure sqlite3 is built with the correct flags
file(GLOB sqlite3_cpp_path support/sqlite3.c)
set_source_files_properties(support/sqlite3.c PROPERTIES
    COMPILE_FLAGS "-DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION"
)

# import enet
set(ENET_SOURCE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/enet)
add_subdirectory(${ENET_SOURCE_DIRECTORY})

# configure local includes
include_directories(
    ${ENET_SOURCE_DIRECTORY}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/engine
    ${CMAKE_CURRENT_SOURCE_DIR}/game
    ${CMAKE_CURRENT_SOURCE_DIR}/shared
    ${CMAKE_CURRENT_SOURCE_DIR}/support
)

# include the headers for the libraries bundled in ../bin
if(MINGW)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()

# most executables share some of their configuration
# therefore we define our own function to add and configure them
function(add_redeclipse_executable target)
    add_executable(${target} ${ARGN})

    set_target_properties(${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")

    if(APPLE)
        # include framework required in xcode/ code
        find_library(COCOA_LIBRARY Cocoa)
        target_link_libraries(${target} ${COCOA_LIBRARY})
    elseif(WIN32 OR MINGW)
        target_link_libraries(${target} ws2_32 winmm shlwapi)
    endif()

    install(
        TARGETS ${target}
        RUNTIME DESTINATION bin
    )
endfunction()

function(add_windows_rc_file target)
    if(MINGW OR WIN32)
        target_sources(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/redeclipse.rc)
    endif()
endfunction()

# APPNAME is the name of the executable -- determines the executable names and is also needed in configure_file below
set(APPNAME "redeclipse-legacy")

# the specific applications configuration is a bit complex, so it's been split into three separate CMake files
include(cmake/client.cmake)
include(cmake/server.cmake)
include(cmake/genkey.cmake)

# install config for cubescript files (containing the menu definitions etc.)
install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/config
    DESTINATION share/redeclipse-legacy
)

# install config for data files
# the if below allows for testing the install config without having to copy too many files
if(NOT SKIP_INSTALL_DATA)
    install(
        DIRECTORY ${PROJECT_SOURCE_DIR}/data
        DESTINATION share/redeclipse-legacy
        PATTERN ".git*" EXCLUDE
    )
endif()

# install desktop file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/install/nix/redeclipse-legacy.desktop.am
    ${CMAKE_CURRENT_BINARY_DIR}/redeclipse-legacy.desktop
    @ONLY
)
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/redeclipse-legacy.desktop
    DESTINATION share/applications
)

# TODO: install appstream data

# install icons
foreach(res IN ITEMS 16 32 48 64 128)
    file(
        COPY ${CMAKE_CURRENT_SOURCE_DIR}/install/nix/redeclipse-legacy_x${res}.png
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/icons/hicolor/${res}/apps/
    )
    file(RENAME
        ${CMAKE_CURRENT_BINARY_DIR}/icons/hicolor/${res}/apps/redeclipse-legacy_x${res}.png
        ${CMAKE_CURRENT_BINARY_DIR}/icons/hicolor/${res}/apps/redeclipse-legacy.png
    )
endforeach()
install(
    DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/icons
    DESTINATION share
)

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


# general CPack configuration
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/cpack_general.cmake)

# add support for release archive generation
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/cpack_archive.cmake)

# must be the last instruction
include(CPack)
