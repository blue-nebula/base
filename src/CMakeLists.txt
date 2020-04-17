# 3.6+ needed for pkg-config's IMPORTED targets
# 3.8+ needed for OpenGL's IMPORTED targets
cmake_minimum_required(VERSION 3.7)

# set compile options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -fomit-frame-pointer -ffast-math -Wall -fsigned-char -fno-exceptions -fno-rtti")

# Use pkg-config to configure some dependencies
find_package(PkgConfig REQUIRED)

pkg_check_modules(zlib zlib REQUIRED IMPORTED_TARGET)
pkg_check_modules(sdl2 sdl2 REQUIRED IMPORTED_TARGET)
pkg_check_modules(SDL2_image SDL2_image REQUIRED IMPORTED_TARGET)
pkg_check_modules(SDL2_mixer SDL2_mixer REQUIRED IMPORTED_TARGET)

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
    elseif(MINGW)
        target_link_libraries(${target} ws2_32 winmm)
    endif()

    install(
        TARGETS ${target}
        RUNTIME DESTINATION bin
    )
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
