cmake_minimum_required(VERSION 3.18)

# Builds the natives for one platform per build folder:
#   Mac:     cmake -S . -B build/mac
#            (both architectures at once, libGameEngineCPlugins.so)
#   Windows: cmake -S . -B build/windows -DCMAKE_TOOLCHAIN_FILE=toolchains/mingw-w64.cmake
#            (cross-compiled on a Mac, libGameEngineCPlugins64.dll)
#            On Windows itself, with Visual Studio's C++ tools:
#            cmake -S . -B build/windows -A x64 && cmake --build build/windows --config Release
#            (build/windows/Release/libGameEngineCPlugins64.dll)
#   iOS:     cmake -S . -B build/ios -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphoneos -DCMAKE_OSX_ARCHITECTURES=arm64
#            (liballnatives.a; use iphonesimulator and "arm64;x86_64" for the simulator)
#   Web:     emcmake cmake -S . -B build/web, with Emscripten's environment set up
#            (load.js, load.wasm and load.data, the fonts; AutoBuild does this)

# On a Mac building for the Mac, build both architectures at once. Set
# before project() so FreeType (added below) is built for both too.
if (CMAKE_HOST_APPLE AND NOT CMAKE_SYSTEM_NAME AND NOT CMAKE_TOOLCHAIN_FILE AND NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif()
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

project(QuorumNatives VERSION 2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(QUORUM_IOS ON)
endif()
if (EMSCRIPTEN)
    set(QUORUM_WEB ON)
endif()

# JNI headers only. jni.h and jni_md.h are the same for both Mac
# architectures and for iOS, so any Mac JDK works; Quorum Studio's is the
# default. Windows needs a Windows JDK's headers, which AutoBuild downloads.
if (APPLE)
    set(QUORUM_JDK "/Applications/QuorumStudio.app/Contents/MacOS/Java/jdk/Contents/Home" CACHE PATH "JDK whose headers are used")
    set(JNI_PLATFORM darwin)
elseif (WIN32)
    set(QUORUM_JDK "${CMAKE_SOURCE_DIR}/download/jdk/windows/x64/jdk-11.0.15+10" CACHE PATH "JDK whose headers are used")
    set(JNI_PLATFORM win32)
endif()
# The web module is called from JavaScript, not Java, so it needs no JNI.
if (NOT QUORUM_WEB AND NOT EXISTS "${QUORUM_JDK}/include/jni.h")
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

if (QUORUM_WEB)
    # The web module: FreeType for Quorum's fonts in the browser. It makes
    # load.js, load.wasm, and load.data, which holds src/web/fonts as
    # /fonts. They go in the standard library's Plugins/javascript, whose
    # FreeTypeStrategy.js calls these functions with cwrap. It is built from
    # the same FreeType as the desktop natives, which has the signed
    # distance fields loadSDFCharC needs.
    set(WEB_EXPORTS "_main,_malloc,_free,_InitializeFreeTypeC,_LoadFontC,_IsFontAvailableC,_SetSizeC,_SetAngleC,_DisposeC,_GetKerningC,_GetUnderlinePositionC,_GetUnderlineThicknessC,_GetLineHeightC,_GetMaximumAscentC,_GetMaximumDescentC,_loadCharC,_loadSDFCharC,_getBitmapC,_getBitmapDataC,_GetLastError")
    add_executable(load src/web/FreeTypeStrategy.cpp)
    set_target_properties(load PROPERTIES SUFFIX ".js")
    target_link_libraries(load PRIVATE freetype)
    target_compile_options(load PRIVATE -O3)
    target_link_options(load PRIVATE
        -O3
        "SHELL:-s WASM=1"
        "SHELL:-s ASSERTIONS=1"
        "SHELL:-s SAFE_HEAP=1"
        "SHELL:-s ALLOW_MEMORY_GROWTH=1"
        "SHELL:-s EXPORTED_FUNCTIONS=${WEB_EXPORTS}"
        "SHELL:-s EXPORTED_RUNTIME_METHODS=cwrap,getValue"
        "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/src/web/fonts@fonts")
    # Nothing else is built for the web.
    return()
endif()

set(NATIVE_SOURCES
    src/main/cpp/BufferUtils.cpp
    src/main/cpp/gdx2d.c
    src/main/cpp/Matrix4.cpp
    src/main/cpp/PixelMap.cpp
    src/main/cpp/FreeTypeStrategy.c
)

# Code shared by the Mac and iOS: text to speech (AVSpeechSynthesizer).
# It uses ARC; the older iOS sources (ObjectAL) don't, so it's set per file.
if (APPLE)
    enable_language(OBJC)
    set(APPLE_SOURCES src/apple/AppleSpeech.m)
    set_source_files_properties(${APPLE_SOURCES} PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
endif()

if (QUORUM_IOS)
    # iOS apps link natives statically: one archive with the natives, the
    # iOS graphics and audio code (ObjectAL), and FreeType. RoboVM links
    # the frameworks (robovm.xml), including AVFoundation for speech.
    file(GLOB_RECURSE IOS_SOURCES CONFIGURE_DEPENDS
        src/ios/*.cpp src/ios/*.m src/ios/*.c)
    add_library(GameEngineCPlugins STATIC ${NATIVE_SOURCES} ${IOS_SOURCES} ${APPLE_SOURCES})
else()
    add_library(GameEngineCPlugins MODULE ${NATIVE_SOURCES} ${APPLE_SOURCES})
    if (APPLE)
        target_link_libraries(GameEngineCPlugins PRIVATE
            "-framework Foundation" "-framework AVFoundation")
    endif()
endif()

target_compile_definitions(GameEngineCPlugins PRIVATE FT2_BUILD_LIBRARY)
target_include_directories(GameEngineCPlugins PRIVATE
    "${QUORUM_JDK}/include"
    "${QUORUM_JDK}/include/${JNI_PLATFORM}")
target_link_libraries(GameEngineCPlugins PRIVATE freetype)
target_compile_options(GameEngineCPlugins PRIVATE -O2)

# The names Quorum loads: libGameEngineCPlugins.so on Mac,
# libGameEngineCPlugins64.dll on Windows, and liballnatives.a on iOS.
if (QUORUM_IOS)
    # A static library doesn't include the libraries it links to, so
    # FreeType's objects are merged into the final archive.
    add_dependencies(GameEngineCPlugins freetype)
    add_custom_command(TARGET GameEngineCPlugins POST_BUILD
        COMMAND libtool -static -o "${CMAKE_BINARY_DIR}/liballnatives.a"
                "$<TARGET_FILE:GameEngineCPlugins>" "$<TARGET_FILE:freetype>"
        COMMENT "Merging FreeType into liballnatives.a")
elseif (APPLE)
    set_target_properties(GameEngineCPlugins PROPERTIES PREFIX "lib" SUFFIX ".so")
elseif (WIN32)
    set_target_properties(GameEngineCPlugins PROPERTIES PREFIX "lib" OUTPUT_NAME "GameEngineCPlugins64")
    if (MINGW)
        # Put the GCC runtime inside the DLL, so it doesn't need
        # libgcc, libstdc++ or libwinpthread DLLs next to it.
        target_link_options(GameEngineCPlugins PRIVATE -static -static-libgcc -static-libstdc++)
    endif()
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
