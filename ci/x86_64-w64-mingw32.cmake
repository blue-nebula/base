# the name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)

set(MINGW_PREFIX x86_64-w64-mingw32 CACHE STRING "" FORCE)

# which compilers to use for C and C++
set(CMAKE_C_COMPILER ${MINGW_PREFIX}-gcc)
set(CMAKE_CXX_COMPILER ${MINGW_PREFIX}-g++)
set(CMAKE_RC_COMPILER ${MINGW_PREFIX}-windres)

# here is the target environment located
set(CMAKE_FIND_ROOT_PATH /usr/${MINGW_PREFIX})

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# make sure libgcc and libstdc++ are linked statically, otherwise we would have to ship them
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -static-libgcc -static-libstdc++")

# set up custom include and library paths for dependencies
set(bundled_libs_dir ${PROJECT_SOURCE_DIR}/src/bundled-libs)

set(ZLIB_INCLUDE_DIR ${bundled_libs_dir}/include)
set(ZLIB_LIBRARY ${bundled_libs_dir}/${MINGW_PREFIX}/lib/zlib1.dll)

set(SDL2_INCLUDE_DIR ${bundled_libs_dir}/include CACHE PATH "" FORCE)
set(SDL2_LIBRARY ${bundled_libs_dir}/${MINGW_PREFIX}/lib/SDL2.dll CACHE PATH "" FORCE)

set(SDL2_IMAGE_INCLUDE_DIR ${bundled_libs_dir}/include CACHE PATH "" FORCE)
set(SDL2_IMAGE_LIBRARY ${bundled_libs_dir}/${MINGW_PREFIX}/lib/SDL2_image.dll CACHE PATH "" FORCE)

set(SDL2_MIXER_INCLUDE_DIR ${bundled_libs_dir}/include CACHE PATH "" FORCE)
set(SDL2_MIXER_LIBRARY ${bundled_libs_dir}/${MINGW_PREFIX}/lib/SDL2_mixer.dll CACHE PATH "" FORCE)
