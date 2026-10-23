# Cross-compiles Windows.dll for 64-bit Windows on a Mac with clang-cl and
# lld-link (Homebrew's llvm and lld), using Microsoft's Windows SDK and C
# runtime that xwin downloads into download/xwin. AutoBuild sets all of this
# up; by hand:
#
#   brew install llvm lld xwin ninja
#   xwin --cache-dir download/xwin-cache --arch x86_64 splat --output download/xwin
#   cmake -S . -B build/windows -G Ninja -DCMAKE_TOOLCHAIN_FILE=toolchains/clang-cl-xwin.cmake
#   cmake --build build/windows
#
# xwin asks you to accept Microsoft's license. download/ is ignored by git:
# Microsoft's files must not be committed.

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_VERSION 10.0)
set(CMAKE_SYSTEM_PROCESSOR AMD64)

# Homebrew's llvm is keg-only (not on the PATH), so look where Homebrew puts it.
set(LLVM_HINTS /opt/homebrew/opt/llvm/bin /usr/local/opt/llvm/bin)
set(LLD_HINTS /opt/homebrew/opt/lld/bin /opt/homebrew/bin /usr/local/opt/lld/bin /usr/local/bin)
find_program(CMAKE_C_COMPILER clang-cl HINTS ${LLVM_HINTS} NO_DEFAULT_PATH REQUIRED)
find_program(CMAKE_CXX_COMPILER clang-cl HINTS ${LLVM_HINTS} NO_DEFAULT_PATH REQUIRED)
find_program(CMAKE_LINKER lld-link HINTS ${LLD_HINTS} ${LLVM_HINTS} REQUIRED)
find_program(CMAKE_AR llvm-lib HINTS ${LLVM_HINTS} NO_DEFAULT_PATH REQUIRED)
find_program(CMAKE_RC_COMPILER llvm-rc HINTS ${LLVM_HINTS} NO_DEFAULT_PATH REQUIRED)
find_program(CMAKE_MT llvm-mt HINTS ${LLVM_HINTS} NO_DEFAULT_PATH REQUIRED)

get_filename_component(XWIN "${CMAKE_CURRENT_LIST_DIR}/../download/xwin" ABSOLUTE)
if (NOT EXISTS "${XWIN}/sdk/include/um/windows.h")
    message(FATAL_ERROR "The Windows SDK isn't in ${XWIN}. Run AutoBuild, or xwin (see the top of this file).")
endif()

set(QUORUM_TARGET_FLAGS "--target=x86_64-pc-windows-msvc -fms-compatibility-version=19.40")
set(QUORUM_XWIN_INCLUDES "/imsvc ${XWIN}/crt/include /imsvc ${XWIN}/sdk/include/ucrt /imsvc ${XWIN}/sdk/include/um /imsvc ${XWIN}/sdk/include/shared /imsvc ${XWIN}/sdk/include/winrt")
set(CMAKE_C_FLAGS_INIT "${QUORUM_TARGET_FLAGS} ${QUORUM_XWIN_INCLUDES}")
set(CMAKE_CXX_FLAGS_INIT "${QUORUM_TARGET_FLAGS} ${QUORUM_XWIN_INCLUDES} /EHsc")
# The resource compiler (FreeType's version resource) needs windows.h too.
set(CMAKE_RC_FLAGS_INIT "/I ${XWIN}/crt/include /I ${XWIN}/sdk/include/ucrt /I ${XWIN}/sdk/include/um /I ${XWIN}/sdk/include/shared")

set(QUORUM_XWIN_LIBS "/libpath:${XWIN}/crt/lib/x86_64 /libpath:${XWIN}/sdk/lib/um/x86_64 /libpath:${XWIN}/sdk/lib/ucrt/x86_64")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${QUORUM_XWIN_LIBS}")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "${QUORUM_XWIN_LIBS}")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "${QUORUM_XWIN_LIBS}")

# xwin downloads only the release C runtime (no libcmtd.lib), so CMake's own
# compiler checks must link in Release too. Debug builds aren't supported here.
set(CMAKE_TRY_COMPILE_CONFIGURATION Release)

# xwin's download has no ATL, so the speech code uses the small stand-ins in
# src/windows/speech/atl instead (see atlbase.h there).
set(QUORUM_ATL_STANDIN ON CACHE BOOL "Use src/windows/speech/atl instead of ATL" FORCE)

# Look for libraries and headers only in xwin's download, never the Mac's.
set(CMAKE_FIND_ROOT_PATH "${XWIN}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
