# release archive option
set(CPACK_NSIS_MUI_ICON "${PROJECT_SOURCE_DIR}/src/install/win/blue-nebula.ico")
set(CPACK_NSIS_MUI_HEADERIMAGE "${PROJECT_SOURCE_DIR}/src/install/win/header.bmp")
set(CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP "${PROJECT_SOURCE_DIR}/src/install/win/finish.bmp")
set(CPACK_NSIS_MUI_UNWELCOMEFINISHPAGE_BITMAP "${CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP}")

set(CPACK_NSIS_DISPLAY_NAME "Blue Nebula")
set(CPACK_NSIS_PACKAGE_NAME "${CPACK_NSIS_DISPLAY_NAME}")

set(CPACK_NSIS_HELP_LINK "https://go.blue-nebula.org/docs")
set(CPACK_NSIS_URL_INFO_ABOUT "https://go.blue-nebula.org/about")
set(CPACK_NSIS_CONTACT "${NSIS_PACKAGE_CONTACT}")

set(CPACK_NSIS_MUI_FINISHPAGE_RUN "${APPNAME}_windows.exe")
set(CPACK_NSIS_UNINSTALL_NAME "Uninstall ${CPACK_NSIS_DISPLAY_NAME}")

set(CPACK_NSIS_INSTALLED_ICON_NAME "bin/${APPNAME}_windows.exe")

# add (or remove) start menu entry and desktop link
set(CPACK_NSIS_MENU_LINKS "bin/${APPNAME}_windows.exe" "${CPACK_NSIS_DISPLAY_NAME}")
set(CPACK_NSIS_CREATE_ICONS_EXTRA "CreateShortCut '$DESKTOP\\\\${CPACK_NSIS_DISPLAY_NAME}.lnk' '$INSTDIR\\\\bin\\\\${APPNAME}_windows.exe'")
set(CPACK_NSIS_DELETE_ICONS_EXTRA "Delete '$DESKTOP\\\\${CPACK_NSIS_DISPLAY_NAME}.lnk'")
