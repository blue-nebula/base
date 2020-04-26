if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    # TODO: install appstream data

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
endif()
