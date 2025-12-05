if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # install appstream data
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/install/nix/blue-nebula.metainfo.xml.am
        ${CMAKE_CURRENT_BINARY_DIR}/blue-nebula.metainfo.xml
        @ONLY
    )
    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/blue-nebula.metainfo.xml
        DESTINATION share/metainfo
    )

    # install desktop file
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/install/nix/blue-nebula.desktop.am
        ${CMAKE_CURRENT_BINARY_DIR}/blue-nebula.desktop
        @ONLY
    )
    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/blue-nebula.desktop
        DESTINATION share/applications
    )

    # install icons
    foreach(res IN ITEMS 16 32 48 64 128 256 512)
        file(
            COPY ${CMAKE_CURRENT_SOURCE_DIR}/install/nix/blue-nebula_${res}.png
            DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/icons/hicolor/${res}x${res}/apps/
        )
        file(RENAME
            ${CMAKE_CURRENT_BINARY_DIR}/icons/hicolor/${res}x${res}/apps/blue-nebula_${res}.png
            ${CMAKE_CURRENT_BINARY_DIR}/icons/hicolor/${res}x${res}/apps/blue-nebula.png
        )
    endforeach()
    install(
        DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/icons
        DESTINATION share
    )
endif()
