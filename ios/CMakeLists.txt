# CMakeLists.txt
#
# Copyright 2013, 2014 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# Written by John Cary <cary@txcorp.com>
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.
#
#
# Say
#
#   cmake CMakeLists.txt
#
# to create a Makefile that builds a static version of the library.
#
# For a dynamic library, use
#
#   cmake CMakeLists.txt -DBUILD_SHARED_LIBS:BOOL=true
#
# For a framework on OS X, use
#
#   cmake CMakeLists.txt -DBUILD_FRAMEWORK:BOOL=true -G Xcode
#
# instead.
#
# For an iOS static library, use
#
#   cmake CMakeLists.txt -DIOS_PLATFORM=OS -G Xcode
#
# or
#
#   cmake CMakeLists.txt -DIOS_PLATFORM=SIMULATOR -G Xcode
#
# Please refer to the cmake manual for further options, in particular, how
# to modify compilation and linking parameters.
#
# Some notes.
#
# . `cmake' will overwrite FreeType's original (top-level) `Makefile' file.
#
# . You can use `cmake' directly on a freshly cloned FreeType git
#   repository.
#
# . `CMakeLists.txt'  is provided as-is since it is not used by the
#   developer team.

cmake_minimum_required(VERSION 2.6)


# CMAKE_TOOLCHAIN_FILE must be set before `project' is called, which
# configures the base build environment and references the toolchain file
if (APPLE)
  if (DEFINED IOS_PLATFORM)
    if (NOT "${IOS_PLATFORM}" STREQUAL "OS"
        AND NOT "${IOS_PLATFORM}" STREQUAL "SIMULATOR")
      message(FATAL_ERROR
        "IOS_PLATFORM must be set to either OS or SIMULATOR")
    endif ()
    if (NOT "${CMAKE_GENERATOR}" STREQUAL "Xcode")
      message(AUTHOR_WARNING
        "You should use Xcode generator with IOS_PLATFORM enabled to get Universal builds.")
    endif ()
    if (BUILD_SHARED_LIBS)
      message(FATAL_ERROR
        "BUILD_SHARED_LIBS can not be on with IOS_PLATFORM enabled")
    endif ()
    if (BUILD_FRAMEWORK)
      message(FATAL_ERROR
        "BUILD_FRAMEWORK can not be on with IOS_PLATFORM enabled")
    endif ()

    # iOS only uses static libraries
    set(BUILD_SHARED_LIBS OFF)

    set(CMAKE_TOOLCHAIN_FILE
      ${PROJECT_SOURCE_DIR}/builds/cmake/iOS.cmake)
  endif ()
else ()
  if (DEFINED IOS_PLATFORM)
    message(FATAL_ERROR "IOS_PLATFORM is not supported on this platform")
  endif ()
endif ()

#added by Nicholas VanCise, fixes march errors upon make in xcode
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")

if (APPLE)
    include_directories("../jdk/mac/x64/jdk-11.0.15+10/Contents/Home/include")
    include_directories("../jdk/mac/x64/jdk-11.0.15+10/Contents/Home/include/darwin")
endif (APPLE)

project(allnativesv2)

if (BUILD_FRAMEWORK)
  if (NOT "${CMAKE_GENERATOR}" STREQUAL "Xcode")
    message(FATAL_ERROR
      "You should use Xcode generator with BUILD_FRAMEWORK enabled")
  endif ()
  set(CMAKE_OSX_ARCHITECTURES "$(ARCHS_STANDARD)")
  set(BUILD_SHARED_LIBS ON)
endif ()

set(VERSION_MAJOR "2")
set(VERSION_MINOR "12")
set(VERSION_PATCH "1")
set(PROJECT_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

# Compiler definitions for building the library
add_definitions(-DFT2_BUILD_LIBRARY)

# Specify library include directories
include_directories("${PROJECT_SOURCE_DIR}/include")

# Create the configuration file
message(STATUS "Creating directory, ${PROJECT_BINARY_DIR}/include/freetype2.")
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/include/freetype2)

# For the auto-generated ftconfig.h file
include_directories(BEFORE "${PROJECT_BINARY_DIR}/include/freetype2")
message(STATUS "Creating ${PROJECT_BINARY_DIR}/include/freetype2/ftconfig.h.")
execute_process(
  COMMAND sed -e "s/FT_CONFIG_OPTIONS_H/<ftoption.h>/" -e "s/FT_CONFIG_STANDARD_LIBRARY_H/<ftstdlib.h>/" -e "s?/undef ?#undef ?"
  INPUT_FILE ${PROJECT_SOURCE_DIR}/builds/unix/ftconfig.in
  OUTPUT_FILE ${PROJECT_BINARY_DIR}/include/freetype2/ftconfig.h
)

file(GLOB PUBLIC_HEADERS "include/ft2build.h" "include/freetype/*.h")
file(GLOB PUBLIC_CONFIG_HEADERS "include/config/*.h")
file(GLOB PRIVATE_HEADERS "include/internal/*.h")

#try to add ios files here (libgameenginecplugin code)
file(GLOB PUBLIC_HEADERS "../../src/ios/*.h")
file(GLOB PUBLIC_HEADERS "../../src/ios/ObjectAL/*.h")
file(GLOB PUBLIC_HEADERS "../../src/ios/ObjectAL/Actions/*.h")
file(GLOB PUBLIC_HEADERS "../../src/ios/ObjectAL/Audiotrack/*.h")
file(GLOB PUBLIC_HEADERS "../../src/ios/ObjectAL/OpenAL/*.h")
file(GLOB PUBLIC_HEADERS "../../src/ios/ObjectAL/Session/*.h")
file(GLOB PUBLIC_HEADERS "../../src/ios/ObjectAL/Support/*.h")

set(BASE_SRCS
  src/autofit/autofit.c
  src/base/ftbase.c
  src/base/ftbbox.c
  src/base/ftbdf.c
  src/base/ftbitmap.c
  src/base/ftcid.c
  src/base/ftfstype.c
  src/base/ftgasp.c
  src/base/ftglyph.c
  src/base/ftgxval.c
  src/base/ftinit.c
  src/base/ftmm.c
  src/base/ftotval.c
  src/base/ftpatent.c
  src/base/ftpfr.c
  src/base/ftstroke.c
  src/base/ftsynth.c
  src/base/fttype1.c
  src/base/ftwinfnt.c
  src/bdf/bdf.c
  src/bzip2/ftbzip2.c
  src/cache/ftcache.c
  src/cff/cff.c
  src/cid/type1cid.c
  src/gzip/ftgzip.c
  src/lzw/ftlzw.c
  src/pcf/pcf.c
  src/pfr/pfr.c
  src/psaux/psaux.c
  src/pshinter/pshinter.c
  src/psnames/psnames.c
  src/raster/raster.c
  src/sdf/sdf.c
  src/sfnt/sfnt.c
  src/smooth/smooth.c
  src/svg/svg.c
  src/truetype/truetype.c
  src/type1/type1.c
  src/type42/type42.c
  src/winfonts/winfnt.c
  ../../src/main/cpp/PixelMap.cpp
  ../../src/main/cpp/Matrix4.cpp
  ../../src/main/cpp/FreeTypeStrategy.c
  ../../src/main/cpp/jpgd_c.cpp
  ../../src/main/cpp/BufferUtils.cpp
  ../../src/main/cpp/gdx2d.c
  ../../src/ios/IOSGraphics.cpp
  ../../src/main/cpp/jpgd.cpp
  ../../src/ios/ObjectAL/OALSimpleAudio.m
  ../../src/ios/ObjectAL/Actions/OALAction.m
  ../../src/ios/ObjectAL/Actions/OALActionManager.m
  ../../src/ios/ObjectAL/Actions/OALAudioActions.m
  ../../src/ios/ObjectAL/Actions/OALUtilityActions.m
  ../../src/ios/ObjectAL/AudioTrack/OALAudioTrack.m
  ../../src/ios/ObjectAL/AudioTrack/OALAudioTrackNotifications.m
  ../../src/ios/ObjectAL/AudioTrack/OALAudioTracks.m
  ../../src/ios/ObjectAL/OpenAL/ALBuffer.m
  ../../src/ios/ObjectAL/OpenAL/ALCaptureDevice.m
  ../../src/ios/ObjectAL/OpenAL/ALChannelSource.m
  ../../src/ios/ObjectAL/OpenAL/ALContext.m
  ../../src/ios/ObjectAL/OpenAL/ALDevice.m
  ../../src/ios/ObjectAL/OpenAL/ALListener.m
  ../../src/ios/ObjectAL/OpenAL/ALSoundSourcePool.m
  ../../src/ios/ObjectAL/OpenAL/ALSource.m
  ../../src/ios/ObjectAL/OpenAL/ALWrapper.m
  ../../src/ios/ObjectAL/OpenAL/OpenALManager.m
  ../../src/ios/ObjectAL/Session/OALAudioSession.m
  ../../src/ios/ObjectAL/Session/OALSuspendHandler.m
  ../../src/ios/ObjectAL/Support/IOSVersion.m
  ../../src/ios/ObjectAL/Support/mach_timing.c
  ../../src/ios/ObjectAL/Support/NSMutableArray+WeakReferences.m
  ../../src/ios/ObjectAL/Support/NSMutableDictionary+WeakReferences.m
  ../../src/ios/ObjectAL/Support/OALAudioFile.m
  ../../src/ios/ObjectAL/Support/OALTools.m
)


list(APPEND BASE_SRCS "src/base/ftsystem.c")

if (WIN32)
  enable_language(RC)
  list(APPEND BASE_SRCS builds/windows/ftdebug.c
                        src/base/ftver.rc)
elseif (WINCE)
  list(APPEND BASE_SRCS builds/wince/ftdebug.c)
else ()
  list(APPEND BASE_SRCS src/base/ftdebug.c)
endif ()

if (BUILD_FRAMEWORK)
  set(BASE_SRCS
    ${BASE_SRCS}
    builds/mac/freetype-Info.plist
  )
endif ()

add_library(allnatives
  ${PUBLIC_HEADERS}
  ${PUBLIC_CONFIG_HEADERS}
  ${PRIVATE_HEADERS}
  ${BASE_SRCS}
)

if (BUILD_FRAMEWORK)
  set_property(SOURCE ${PUBLIC_CONFIG_HEADERS}
    PROPERTY MACOSX_PACKAGE_LOCATION Headers/config
  )
  set_target_properties(allnatives PROPERTIES
    FRAMEWORK TRUE
    MACOSX_FRAMEWORK_INFO_PLIST builds/mac/freetype-Info.plist
    PUBLIC_HEADER "${PUBLIC_HEADERS}"
    XCODE_ATTRIBUTE_INSTALL_PATH "@rpath"
  )
endif ()

# Installations
# Note the trailing slash in the argument to the `DIRECTORY' directive
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/
  DESTINATION include/freetype2
  PATTERN "internal" EXCLUDE
)
install(TARGETS allnatives
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  FRAMEWORK DESTINATION Library/Frameworks
)

# Packaging
# CPack version numbers for release tarball name.
set(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH}})
if (NOT DEFINED CPACK_PACKAGE_DESCRIPTION_SUMMARY)
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${CMAKE_PROJECT_NAME}")
endif ()
if (NOT DEFINED CPACK_SOURCE_PACKAGE_FILE_NAME)
  set(CPACK_SOURCE_PACKAGE_FILE_NAME
    "${CMAKE_PROJECT_NAME}-${PROJECT_VERSION}-r${PROJECT_REV}"
    CACHE INTERNAL "tarball basename"
  )
endif ()
set(CPACK_SOURCE_GENERATOR TGZ)
set(CPACK_SOURCE_IGNORE_FILES
  "/CVS/;/.svn/;.swp$;.#;/#;/build/;/serial/;/ser/;/parallel/;/par/;~;/preconfig.out;/autom4te.cache/;/.config")
set(CPACK_GENERATOR TGZ)
include(CPack)

# add make dist target
add_custom_target(dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

# eof
