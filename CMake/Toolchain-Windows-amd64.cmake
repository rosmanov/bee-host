# The name of the target operating system
set(CMAKE_SYSTEM_NAME Windows)

set(BEECTL_HOST x86_64-w64-mingw32)

# Which compilers to use for C and C++
set(CMAKE_C_COMPILER ${BEECTL_HOST}-gcc)
set(CMAKE_CXX_COMPILER ${BEECTL_HOST}-g++)
set(CMAKE_RC_COMPILER ${BEECTL_HOST}-windres)
set(CMAKE_AR ${BEECTL_HOST}-ar)

set(CMAKE_C_FLAGS "-static" CACHE STRING "" FORCE)
link_libraries(-static-libgcc -static-libstdc++)

# Target environment located
set(CMAKE_FIND_ROOT_PATH /usr/${BEECTL_HOST})

# Adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
