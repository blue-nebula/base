cmake_minimum_required(VERSION 3.0)

# avoid annoying warnings due to use of a ${PREFIX} variable in the toolchain(s)
# http://www.steveire.com/embracing-modern-cmake.pdf
if(POLICY CMP0053)
    cmake_policy(SET CMP0053 NEW)
endif()

project("Red Eclipse Legacy" C CXX)

add_subdirectory(src)
