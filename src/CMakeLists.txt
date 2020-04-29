# 3.6+ needed for pkg-config's IMPORTED targets, as well as IMPORTED targets for zlib
# 3.8+ needed for OpenGL's IMPORTED targets
cmake_minimum_required(VERSION 3.8)

# set compile options
set(COMMON_C_FLAGS "-Wall -fsigned-char")
# note: "SQLite will not work correctly with the -ffast-math option of GCC" -> we'll only enable this flag for the C++ codebase
# TODO: drop sqlite dependency
set(COMMON_CXX_FLAGS "-fno-rtti -ffast-math -fno-exceptions")

if(CMAKE_BUILD_TYPE MATCHES RELEASE)
    message(STATUS "Release build detected, adding release flags")
    set(COMMON_C_FLAGS "${CMAKE_CXX_FLAGS} -O3 -fomit-frame-pointer")
endif()

option(PEDANTIC "Enable pedantic mode")
if(PEDANTIC)
    message(STATUS "Enabling pedantic mode")
    if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
        set(COMMON_C_FLAGS "${ADDITIONAL_BUILD_FLAGS} -Wextra")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES Clang)
        set(COMMON_C_FLAGS "${ADDITIONAL_BUILD_FLAGS} -Wpedantic")
    else()
        message(FATAL_ERROR "Pedantic mode requested, but no configuration available for compiler identification '${CMAKE_CXX_COMPILER_ID}'")
    endif()
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_C_FLAGS} ${COMMON_CXX_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_C_FLAGS}")

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

# override from outside or in the platform specific code to exclude targets from being built
set(BUILD_CLIENT ON)
set(BUILD_SERVER ON)
set(BUILD_GENKEY ON)

# platform specific code
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    set(BIN_SUFFIX "_linux")
elseif(APPLE)
    set(BIN_SUFFIX "_osx")

    # TODO: fix genkey on macOS
    set(BUILD_GENKEY OFF)
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
include(cmake/build_client.cmake)
include(cmake/build_server.cmake)
include(cmake/build_genkey.cmake)

# install config directory with the cubescript files (containing the menu definitions etc.)
install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/config
    DESTINATION share/redeclipse-legacy
)

# install config for data files
# the if below allows for testing the install config without having to copy too many files
if(SKIP_INSTALL_DATA)
    message(WARNING "not installing any game data")
else()
    install(
        DIRECTORY ${PROJECT_SOURCE_DIR}/data
        DESTINATION share/redeclipse-legacy
        PATTERN ".git*" EXCLUDE
    )
endif()

# install configs were moved into separate CMake files
include(cmake/install_nix.cmake)
include(cmake/install_win.cmake)

# general CPack configuration
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/cpack_general.cmake)

# add support for release archive generation
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/cpack_archive.cmake)

# must be the last instruction
include(CPack)
