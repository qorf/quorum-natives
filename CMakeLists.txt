cmake_minimum_required(VERSION 3.18)

# Builds the natives for one platform per build folder:
#   Mac:     cmake -S . -B build/mac
#            (Mac.so, one universal file with both architectures)
#   Windows: cmake -S . -B build/windows -G Ninja -DCMAKE_TOOLCHAIN_FILE=toolchains/clang-cl-xwin.cmake
#            (cross-compiled on a Mac with clang-cl, Windows.dll: the game
#            natives, accessibility and speech; see the toolchain file)
#            On Windows itself, with Visual Studio's C++ tools and ATL:
#            cmake -S . -B build/windows -A x64 && cmake --build build/windows --config Release
#            (build/windows/Release/Windows.dll)
#            Both need WIL in download/wil, which AutoBuild downloads.
#   iOS:     cmake -S . -B build/ios -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphoneos -DCMAKE_OSX_ARCHITECTURES=arm64
#            (liballnatives.a; use iphonesimulator and "arm64;x86_64" for the simulator)
#   Android: cmake -S . -B build/android-arm64-v8a -G Ninja -DCMAKE_TOOLCHAIN_FILE=<ndk>/build/cmake/android.toolchain.cmake
#                  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-35 -DANDROID_STL=c++_static
#            (libGameEngineCPlugins.so; once per ABI, x86_64 too)
#   Web:     emcmake cmake -S . -B build/web, with Emscripten's environment set up
#            (load.js, load.wasm and load.data, the fonts; AutoBuild does this)
#
# iOS and Android also get Quorum's own Vulkan layer (src/vulkan), which calls
# Vulkan directly: MoltenVK, merged into the iOS archive, or the phone's own
# Vulkan on Android. It uses the Vulkan headers, VMA and glslang (to compile
# shaders), which AutoBuild downloads into download/, as it does MoltenVK.

# On a Mac building for the Mac, build both architectures at once. Set
# before project() so FreeType (added below) is built for both too.
if (CMAKE_HOST_APPLE AND NOT CMAKE_SYSTEM_NAME AND NOT CMAKE_TOOLCHAIN_FILE AND NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "" FORCE)
endif()
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()
# On Windows, put the C and C++ runtime inside Windows.dll (/MT), FreeType's
# code included, so users need no Visual C++ Redistributable. Set before
# project() so it applies to everything.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(QuorumNatives VERSION 2.0 LANGUAGES C CXX)

if (WIN32 AND NOT MSVC)
    message(FATAL_ERROR "Windows.dll is built with clang-cl (on a Mac; see toolchains/clang-cl-xwin.cmake) or Visual Studio (on Windows). MinGW isn't supported: the accessibility code needs Microsoft's WIL.")
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(QUORUM_IOS ON)
endif()
if (ANDROID)
    set(QUORUM_ANDROID ON)
endif()
if (EMSCRIPTEN)
    set(QUORUM_WEB ON)
endif()

# JNI headers only. jni.h and jni_md.h are the same for both Mac
# architectures and for iOS, so any Mac JDK works; Quorum Studio's is the
# default. Windows needs a Windows JDK's headers, which AutoBuild downloads.
# Android's NDK has its own jni.h.
if (APPLE)
    set(QUORUM_JDK "/Applications/QuorumStudio.app/Contents/MacOS/Java/jdk/Contents/Home" CACHE PATH "JDK whose headers are used")
    set(JNI_PLATFORM darwin)
elseif (WIN32)
    set(QUORUM_JDK "${CMAKE_SOURCE_DIR}/download/jdk/windows/x64/jdk-11.0.15+10" CACHE PATH "JDK whose headers are used")
    set(JNI_PLATFORM win32)
endif()
# The web module is called from JavaScript, not Java, so it needs no JNI.
if (NOT QUORUM_WEB AND NOT QUORUM_ANDROID AND NOT EXISTS "${QUORUM_JDK}/include/jni.h")
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

# Windows only: accessibility (UI Automation, src/windows/accessibility) and
# speech (SAPI, src/windows/speech), in the same Windows.dll.
if (WIN32)
    file(GLOB WINDOWS_SOURCES CONFIGURE_DEPENDS
        src/windows/accessibility/*.cpp
        src/windows/speech/*.cpp)
    set(QUORUM_WIL "${CMAKE_SOURCE_DIR}/download/wil/include")
    if (NOT EXISTS "${QUORUM_WIL}/wil/com.h")
        message(FATAL_ERROR "WIL is missing from download/wil. Run AutoBuild once to download it.")
    endif()
endif()

# iOS and Android: Quorum's Vulkan layer. The Java side is
# plugins.quorum.Libraries.Game.Graphics.Vulkan.Native; the desktop uses
# LWJGL instead, so none of this is built for it.
if (QUORUM_IOS OR QUORUM_ANDROID)
    set(QUORUM_VULKAN_HEADERS "${CMAKE_SOURCE_DIR}/download/vulkan-headers/include")
    set(QUORUM_VMA "${CMAKE_SOURCE_DIR}/download/vma/include")
    set(QUORUM_GLSLANG "${CMAKE_SOURCE_DIR}/download/glslang")
    if (NOT EXISTS "${QUORUM_VULKAN_HEADERS}/vulkan/vulkan.h" OR NOT EXISTS "${QUORUM_VMA}/vk_mem_alloc.h"
            OR NOT EXISTS "${QUORUM_GLSLANG}/CMakeLists.txt")
        message(FATAL_ERROR "The Vulkan headers, VMA or glslang are missing from download/. Run AutoBuild once to download them.")
    endif()
    file(GLOB VULKAN_SOURCES CONFIGURE_DEPENDS src/vulkan/*.cpp)

    # glslang compiles the GLSL shaders to SPIR-V when a game runs, as
    # shaderc does on the desktop (shaderc is built on glslang). Only the
    # library is built: no tools, tests, HLSL or optimizer (Quorum's shaders
    # are compiled without optimization on the desktop too).
    set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
    set(ENABLE_HLSL OFF CACHE BOOL "" FORCE)
    set(ENABLE_OPT OFF CACHE BOOL "" FORCE)
    set(ENABLE_PCH OFF CACHE BOOL "" FORCE)
    set(ENABLE_SPIRV ON CACHE BOOL "" FORCE)
    set(BUILD_EXTERNAL OFF CACHE BOOL "" FORCE)
    set(GLSLANG_TESTS OFF CACHE BOOL "" FORCE)
    set(GLSLANG_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory(download/glslang EXCLUDE_FROM_ALL)
endif()

if (QUORUM_IOS)
    # iOS apps link natives statically: one archive with the natives, the
    # iOS graphics and audio code (ObjectAL), FreeType, and for Vulkan,
    # glslang and MoltenVK. RoboVM links the frameworks (robovm.xml),
    # including AVFoundation for speech, and Metal, QuartzCore and IOSurface
    # for MoltenVK.
    file(GLOB_RECURSE IOS_SOURCES CONFIGURE_DEPENDS
        src/ios/*.cpp src/ios/*.m src/ios/*.c)
    add_library(GameEngineCPlugins STATIC ${NATIVE_SOURCES} ${IOS_SOURCES} ${APPLE_SOURCES} ${VULKAN_SOURCES})

    # MoltenVK's static library, the slice for phones or for the simulator.
    # CMake turns "iphonesimulator" into the SDK's path (iPhoneSimulator...).
    if (CMAKE_OSX_SYSROOT MATCHES "[Ss]imulator")
        set(QUORUM_MOLTENVK_SLICE ios-arm64_x86_64-simulator)
    else()
        set(QUORUM_MOLTENVK_SLICE ios-arm64)
    endif()
    set(QUORUM_MOLTENVK "${CMAKE_SOURCE_DIR}/download/moltenvk-package/MoltenVK/static/MoltenVK.xcframework/${QUORUM_MOLTENVK_SLICE}/libMoltenVK.a")
    if (NOT EXISTS "${QUORUM_MOLTENVK}")
        message(FATAL_ERROR "MoltenVK is missing: ${QUORUM_MOLTENVK}. Run AutoBuild once to download it.")
    endif()
elseif (QUORUM_ANDROID)
    # One shared library per ABI, loaded by System.loadLibrary("GameEngineCPlugins").
    add_library(GameEngineCPlugins SHARED ${NATIVE_SOURCES} ${VULKAN_SOURCES})
else()
    add_library(GameEngineCPlugins MODULE ${NATIVE_SOURCES} ${APPLE_SOURCES} ${WINDOWS_SOURCES})
    if (APPLE)
        target_link_libraries(GameEngineCPlugins PRIVATE
            "-framework Foundation" "-framework AVFoundation")
    endif()
endif()

target_compile_definitions(GameEngineCPlugins PRIVATE FT2_BUILD_LIBRARY)
if (NOT QUORUM_ANDROID)
    target_include_directories(GameEngineCPlugins PRIVATE
        "${QUORUM_JDK}/include"
        "${QUORUM_JDK}/include/${JNI_PLATFORM}")
endif()
target_link_libraries(GameEngineCPlugins PRIVATE freetype)
target_compile_options(GameEngineCPlugins PRIVATE -O2)

if (QUORUM_IOS OR QUORUM_ANDROID)
    # VMA's own code is compiled in src/vulkan/NativeVulkanMemoryAllocator.cpp,
    # calling Vulkan's functions directly, as the rest of src/vulkan does.
    set_target_properties(GameEngineCPlugins PROPERTIES CXX_STANDARD 17)
    target_include_directories(GameEngineCPlugins PRIVATE
        "${QUORUM_VULKAN_HEADERS}"
        "${QUORUM_VMA}"
        "${QUORUM_GLSLANG}"
        src/vulkan)
    # VMA uses at most Vulkan 1.3's functions, Quorum's minimum: Android's
    # Vulkan library has them from Android 13, so they link on Android 15.
    target_compile_definitions(GameEngineCPlugins PRIVATE
        VMA_STATIC_VULKAN_FUNCTIONS=1
        VMA_DYNAMIC_VULKAN_FUNCTIONS=0
        VMA_VULKAN_VERSION=1003000)
    if (QUORUM_IOS)
        target_compile_definitions(GameEngineCPlugins PRIVATE VK_USE_PLATFORM_METAL_EXT)
    else()
        target_compile_definitions(GameEngineCPlugins PRIVATE VK_USE_PLATFORM_ANDROID_KHR)
    endif()
    target_link_libraries(GameEngineCPlugins PRIVATE glslang glslang-default-resource-limits)
endif()

# The names Quorum loads: Mac.so on Mac (universal, arm64 and x86_64),
# Windows.dll on Windows, liballnatives.a on iOS, and
# libGameEngineCPlugins.so on Android.
if (QUORUM_IOS)
    # A static library doesn't include the libraries it links to, so
    # FreeType's, glslang's and MoltenVK's objects are merged into the final
    # archive.
    add_dependencies(GameEngineCPlugins freetype glslang glslang-default-resource-limits)
    add_custom_command(TARGET GameEngineCPlugins POST_BUILD
        COMMAND libtool -static -o "${CMAKE_BINARY_DIR}/liballnatives.a"
                "$<TARGET_FILE:GameEngineCPlugins>" "$<TARGET_FILE:freetype>"
                "$<TARGET_FILE:glslang>" "$<TARGET_FILE:glslang-default-resource-limits>"
                "${QUORUM_MOLTENVK}"
        COMMENT "Merging FreeType, glslang and MoltenVK into liballnatives.a")
elseif (QUORUM_ANDROID)
    # log for messages, android for the window a Vulkan surface is made on,
    # and the phone's Vulkan.
    target_link_libraries(GameEngineCPlugins PRIVATE log android vulkan)
    # Google Play needs native libraries that work with 16 KB memory pages.
    # NDK r28 and later align them that way; this makes sure.
    target_link_options(GameEngineCPlugins PRIVATE "-Wl,-z,max-page-size=16384")
    # The NDK's toolchain compiles with debug information even for Release,
    # which makes the library about ten times larger. Gradle can only strip
    # it when an NDK is set up for the app, and Quorum's isn't, so it's
    # stripped here. The JNI functions stay (they're dynamic symbols).
    target_link_options(GameEngineCPlugins PRIVATE "-Wl,--strip-all")
elseif (APPLE)
    set_target_properties(GameEngineCPlugins PROPERTIES PREFIX "" OUTPUT_NAME "Mac" SUFFIX ".so")
elseif (WIN32)
    # The accessibility code needs C++17 (WIL).
    set_target_properties(GameEngineCPlugins PROPERTIES PREFIX "" OUTPUT_NAME "Windows" CXX_STANDARD 17)
    target_compile_definitions(GameEngineCPlugins PRIVATE UNICODE _UNICODE)
    target_include_directories(GameEngineCPlugins PRIVATE
        "${QUORUM_WIL}"
        src/windows/accessibility
        src/windows/speech)
    if (QUORUM_ATL_STANDIN)
        # Small stand-ins for the few ATL classes the speech code uses, when
        # ATL isn't available (the clang-cl build on a Mac).
        target_include_directories(GameEngineCPlugins PRIVATE src/windows/speech/atl)
    endif()
    target_link_libraries(GameEngineCPlugins PRIVATE
        UIAutomationCore sapi advapi32 ole32 oleaut32 user32 uuid
        "comsuppw$<$<CONFIG:Debug>:d>")
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
