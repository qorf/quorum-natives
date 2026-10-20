cmake_minimum_required(VERSION 3.16)

# Build both Mac architectures at once. Set before project() so FreeType
# (added below) is built for both too.
if (APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif()
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

project(QuorumNatives VERSION 2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# JNI headers only. jni.h and jni_md.h are the same for both Mac
# architectures, so any JDK works; Quorum Studio's is the default.
if (APPLE)
    set(QUORUM_JDK "/Applications/QuorumStudio.app/Contents/MacOS/Java/jdk/Contents/Home" CACHE PATH "JDK whose headers are used")
    set(JNI_PLATFORM darwin)
elseif (WIN32)
    set(QUORUM_JDK "${CMAKE_SOURCE_DIR}/download/jdk/windows/x64/jdk-11.0.15+10" CACHE PATH "JDK whose headers are used")
    set(JNI_PLATFORM win32)
endif()
if (NOT EXISTS "${QUORUM_JDK}/include/jni.h")
    message(FATAL_ERROR "No jni.h under ${QUORUM_JDK}. Pass -DQUORUM_JDK=<a JDK home>.")
endif()

# FreeType, built from the downloaded source as part of this build, with
# every optional dependency off.
set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
# FreeType 2.13.0 asks for CMake 3.0, which CMake 4 refuses without this.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
if (NOT EXISTS "${CMAKE_SOURCE_DIR}/download/freetype/CMakeLists.txt")
    message(FATAL_ERROR "FreeType is missing. Run AutoBuild once to download it.")
endif()
file(READ "${CMAKE_SOURCE_DIR}/download/freetype/CMakeLists.txt" FREETYPE_CMAKE)
if (FREETYPE_CMAKE MATCHES "allnativesv2")
    message(FATAL_ERROR "download/freetype/CMakeLists.txt is the iOS build's copy. Delete download/freetype and run AutoBuild again to extract FreeType fresh.")
endif()
add_subdirectory(download/freetype EXCLUDE_FROM_ALL)

add_library(GameEngineCPlugins MODULE
    src/main/cpp/BufferUtils.cpp
    src/main/cpp/gdx2d.c
    src/main/cpp/Matrix4.cpp
    src/main/cpp/PixelMap.cpp
    src/main/cpp/FreeTypeStrategy.c
)
target_compile_definitions(GameEngineCPlugins PRIVATE FT2_BUILD_LIBRARY)
target_include_directories(GameEngineCPlugins PRIVATE
    "${QUORUM_JDK}/include"
    "${QUORUM_JDK}/include/${JNI_PLATFORM}")
target_link_libraries(GameEngineCPlugins PRIVATE freetype)
target_compile_options(GameEngineCPlugins PRIVATE -O2)

# The names Game.java loads: libGameEngineCPlugins.so on Mac,
# libGameEngineCPlugins64.dll on Windows.
if (APPLE)
    set_target_properties(GameEngineCPlugins PROPERTIES PREFIX "lib" SUFFIX ".so")
elseif (WIN32)
    set_target_properties(GameEngineCPlugins PROPERTIES PREFIX "lib" OUTPUT_NAME "GameEngineCPlugins64")
endif()

# Image loader tests: decode files and check formats, no JNI or Java needed.
option(QUORUM_NATIVES_TESTS "Build the image loader tests" OFF)
if (QUORUM_NATIVES_TESTS)
    enable_testing()
    add_executable(ImageLoaderTests tests/ImageLoaderTests.c src/main/cpp/gdx2d.c)
    target_compile_definitions(ImageLoaderTests PRIVATE NOJNI)
    target_include_directories(ImageLoaderTests PRIVATE src/main/cpp)
    add_test(NAME ImageLoader COMMAND ImageLoaderTests ${CMAKE_SOURCE_DIR}/tests/images)
endif()
