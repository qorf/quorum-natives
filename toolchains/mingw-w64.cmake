# Cross-compiles the natives for 64-bit Windows from a Mac (or Linux) with
# MinGW-w64. Install it with: brew install mingw-w64
#
#   cmake -S . -B build/windows -DCMAKE_TOOLCHAIN_FILE=toolchains/mingw-w64.cmake

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(MINGW_PREFIX x86_64-w64-mingw32)
set(MINGW_SEARCH_PATHS /opt/homebrew/bin /usr/local/bin /usr/bin)

find_program(CMAKE_C_COMPILER ${MINGW_PREFIX}-gcc PATHS ${MINGW_SEARCH_PATHS} REQUIRED)
find_program(CMAKE_CXX_COMPILER ${MINGW_PREFIX}-g++ PATHS ${MINGW_SEARCH_PATHS} REQUIRED)
find_program(CMAKE_RC_COMPILER ${MINGW_PREFIX}-windres PATHS ${MINGW_SEARCH_PATHS} REQUIRED)

# Look for libraries and headers only in the MinGW install, never the Mac's.
get_filename_component(MINGW_BIN "${CMAKE_C_COMPILER}" DIRECTORY)
get_filename_component(MINGW_ROOT "${MINGW_BIN}/.." ABSOLUTE)
set(CMAKE_FIND_ROOT_PATH "${MINGW_ROOT}/${MINGW_PREFIX}" "${MINGW_ROOT}/toolchain-x86_64/${MINGW_PREFIX}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
