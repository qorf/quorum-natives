cmake_minimum_required(VERSION 3.0.0)
SET(ARCH OFF)


project(QuorumNatives VERSION 2.0)

add_definitions(-DFT2_BUILD_LIBRARY)

SET(CMAKE_CXX_FLAGS "-O2 -std=c++11")

SET(CMAKE_EXE_LINKER_FLAGS "-v")
if (APPLE)
    message("Building for Mac OS X ")
    if(ARCH) 
        message("Building for ARM")
        set(CMAKE_OSX_ARCHITECTURES "arm64")
        include_directories("download/jdk/mac/aarch64/jdk-11.0.15+10/Contents/Home/include")
        include_directories("download/jdk/mac/aarch64/jdk-11.0.15+10/Contents/Home/include/darwin")
    else()
        message("Building for x86_64")
        set(CMAKE_OSX_ARCHITECTURES "x86_64")
        include_directories("download/jdk/mac/x64/jdk-11.0.15+10/Contents/Home/include")
        include_directories("download/jdk/mac/x64/jdk-11.0.15+10/Contents/Home/include/darwin")
    endif()
    
endif (APPLE)

if (WIN32)
    include_directories("download/jdk/windows/x64/jdk-11.0.15+10/include/")
    include_directories("download/jdk/windows/x64/jdk-11.0.15+10/include/win32")
endif (WIN32)


#include_directories("src/main/cpp/freetype/include")
include_directories("download/freetype/include")
add_library(
    libGameEngineCPlugins64 

    # Sets the library as a shared library.
    MODULE

    # Provides a relative path to your source file(s).
    src/main/cpp/BufferUtils.cpp
    src/main/cpp/BufferUtils.h
    src/main/cpp/gdx2d.c
    src/main/cpp/gdx2d.h
    src/main/cpp/jpgd.cpp
    src/main/cpp/jpgd.h
    src/main/cpp/jpgd_c.cpp
    src/main/cpp/jpgd_c.h
    src/main/cpp/Matrix4.cpp
    src/main/cpp/Matrix4.h
    src/main/cpp/PixelMap.cpp
    src/main/cpp/PixelMap.h
    src/main/cpp/stb_image.h
    src/main/cpp/FreeTypeStrategy.c
    src/main/cpp/FreeTypeStrategy.h
    )

    if (APPLE)
        if(ARCH) 
            message("Linking for ARM")
            target_link_libraries(libGameEngineCPlugins64 ${CMAKE_SOURCE_DIR}/download/freetype/FreeType_Release/freetype_Arm.a)
        else()
            message("Linking for x86_64")
            target_link_libraries(libGameEngineCPlugins64 ${CMAKE_SOURCE_DIR}/download/freetype/FreeType_Release/freetype_x86_64.a)
        endif()
    endif (APPLE)

    if (WIN32)
        target_link_libraries(libGameEngineCPlugins64 ${CMAKE_SOURCE_DIR}/download/freetype/objs/freetype.lib)
    endif (WIN32)