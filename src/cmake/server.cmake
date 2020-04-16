# the server requires less source files than the client, so we pick them by hand
file(GLOB server_sources
    shared/crypto.cpp
    shared/geom.cpp
    shared/stream.cpp
    shared/tools.cpp
    shared/zip.cpp
    support/sqlite3.c
    engine/command.cpp
    engine/irc.cpp
    engine/master.cpp
    engine/server.cpp
    game/server.cpp
)

# dependencies are imported globally in src/CMakeLists.txt
# we can just list them by name here
set(server_deps PkgConfig::zlib enet)

# platform specific code
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # server only needs rt in addition to its other deps
    list(APPEND server_deps rt)
elseif(APPLE)
    # build OS X specific Objective-C code
    file(GLOB mac_server_sources
        xcode/macutils.mm
    )
    list(APPEND server_sources ${mac_server_sources})
endif()

# add the server executable and link it to enet
add_redeclipse_executable(redeclipse_server${BIN_SUFFIX} ${server_sources})

# server only depends on enet
target_link_libraries(redeclipse_server${BIN_SUFFIX} ${server_deps})

# (define STANDALONE to "notify" the preprocessor that the server is built this time)
set_target_properties(redeclipse_server${BIN_SUFFIX} PROPERTIES
    COMPILE_FLAGS "-DSTANDALONE"
)

