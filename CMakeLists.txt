cmake_minimum_required(VERSION 3.0.0)
project(QuorumNatives VERSION 0.1.0)

add_definitions(-DFT2_BUILD_LIBRARY)

SET(CMAKE_CXX_FLAGS "-O2 -std=c++11")

SET(CMAKE_EXE_LINKER_FLAGS "-v")
include_directories("/Users/stefik/Repositories/quorum-natives/jdk/mac/x64/jdk-11.0.15+10/Contents/Home/include")
include_directories("/Users/stefik/Repositories/quorum-natives/jdk/mac/x64/jdk-11.0.15+10/Contents/Home/include/darwin")

include_directories("src/main/cpp/freetype/include")

file(GLOB PUBLIC_HEADERS "src/main/cpp/freetype/include/*.h")
file(GLOB PUBLIC_CONFIG_HEADERS "src/main/cpp/freetype/include/config/*.h")
file(GLOB PRIVATE_HEADERS "src/main/cpp/freetype/include/internal/*.h")

add_library(
    GameEngineCPlugins 

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
    ${PUBLIC_HEADERS}
  ${PUBLIC_CONFIG_HEADERS}
  ${PRIVATE_HEADERS}
    )

    include_directories("src/main/cpp/freetype/src/truetype")
    include_directories("src/main/cpp/freetype/src/sfnt")
    include_directories("src/main/cpp/freetype/src/autofit")
    include_directories("src/main/cpp/freetype/src/smooth")
    include_directories("src/main/cpp/freetype/src/raster")
    include_directories("src/main/cpp/freetype/src/psaux")
    include_directories("src/main/cpp/freetype/src/psnames")
    target_link_libraries(GameEngineCPlugins ${CMAKE_SOURCE_DIR}/libfreetype.a)