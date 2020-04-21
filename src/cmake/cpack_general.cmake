# general CPack options

#set(PROJECT_VERSION ${APPIMAGELAUNCHER_VERSION})

# package them all in a single package, otherwise cpack would generate one package per component by default
# https://cmake.org/cmake/help/v3.0/module/CPackComponent.html#variable:CPACK_COMPONENTS_GROUPING
set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)

# global options
set(CPACK_PACKAGE_CONTACT "Red Eclipse Legacy Team")
set(CPACK_PACKAGE_HOMEPAGE "https://github.com/redeclipse-legacy")
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
set(CPACK_PACKAGE_NAME red-eclipse-legacy)
set(CPACK_PACKAGE_VERSION ${_CPACK_VERSION})

