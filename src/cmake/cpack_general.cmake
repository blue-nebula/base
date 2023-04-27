# general CPack options

#set(PROJECT_VERSION ${APPIMAGELAUNCHER_VERSION})

# package them all in a single package, otherwise cpack would generate one package per component by default
# https://cmake.org/cmake/help/v3.0/module/CPackComponent.html#variable:CPACK_COMPONENTS_GROUPING
set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)

# global options
set(CPACK_PACKAGE_VENDOR "Blue Nebula Team")
set(CPACK_PACKAGE_CONTACT "${CPACK_PACKAGE_VENDOR}")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://blue-nebula.org")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")
#set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE.txt")

# we gather our version number from git
execute_process(
    COMMAND git describe --tags HEAD
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    OUTPUT_VARIABLE _CPACK_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# CMake composes file names using a <package_name>-<package_version>-<platform>.<ext> scheme
set(CPACK_PACKAGE_NAME blue-nebula)
set(CPACK_PACKAGE_VERSION ${_CPACK_VERSION})

set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/doc/all-licenses.txt")
set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")

# used by installers like NSIS
# we do not want this directory name to contain a version number typically, as we don't support multiple installations in parallel
# people can use the generated archive if need be
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Blue Nebula")
