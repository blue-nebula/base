cmake_minimum_required(VERSION 3.0)

# avoid annoying warnings due to use of a ${PREFIX} variable in the toolchain(s)
# http://www.steveire.com/embracing-modern-cmake.pdf
if(POLICY CMP0053)
    cmake_policy(SET CMP0053 NEW)
endif()

project("Red Eclipse Legacy" C CXX)

# require at least C++14 and C11
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(src)
