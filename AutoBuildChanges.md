# AutoBuild Changes: a one-step Mac build

Goal: running the AutoBuild Quorum project on a Mac produces both Mac
libraries and copies them into the standard library. No VS Code, no hand
edits to `CMakeLists.txt`, no Terminal steps, and no renaming files.

**Applied 2026-10-20:** Changes 1, 2 and 3. `CMakeLists.txt` and
`Main.quorum` also deal with a `download/freetype` folder that still has
the iOS `CMakeLists.txt` from an older run: the script deletes it so it is
extracted again, and CMake stops with a clear message. Changes 4 (README)
and 5 (tests) are not applied.

Status: the CMake part below was **built and checked** on this machine
(2026-10-20, CMake 4.4.3, Xcode clang, Apple Silicon). The Quorum part is
written against the current `Process`, `File` and `Properties` libraries
but has **not been run**.

## Why the Mac build is hard today

From the README's 17 steps and `AutoBuild/SourceCode/Main.quorum`:

1. **The FreeType architecture follows the JVM that runs AutoBuild.**
   `ConfigureFreetype` passes `-target arm64-apple-macos`. Autoconf reads
   that as `--target`, which only matters when building compilers, so it
   has no effect. `./configure` builds for the machine the process runs
   on. With an x86_64 JDK under Rosetta, you get x86_64. With Quorum
   Studio's current arm64 JDK, you get arm64. (Today's run made an arm64
   `freetype_Arm.a`.) This is the README's "x86_64 only, for reasons that
   are not clear", and it's why step 6 is a manual Terminal command.
2. **Switching architectures reuses old objects.** `CleanFreetype` is
   commented out in `DoMac`, so a second configure-and-make can mix object
   files from both architectures.
3. **`CMakeLists.txt` is edited by hand** (`SET(ARCH OFF/ON)`) and built
   twice in VS Code. The output, `liblibGameEngineCPlugins64.so`, is then
   renamed by hand.
4. **The current CMake rejects the project.** CMake 4 dropped support for
   `cmake_minimum_required` below 3.5. Both our `CMakeLists.txt` (3.0.0)
   and FreeType 2.13.0's (3.0) now stop with "Compatibility with CMake <
   3.5 has been removed". Until this is fixed, configure with
   `-DCMAKE_POLICY_VERSION_MINIMUM=3.5`.
5. **Three JDKs are downloaded (about 600 MB) just for `jni.h`.** On Mac,
   `jni.h` and `jni_md.h` are the same for both architectures, and Quorum
   Studio already ships a JDK.
6. **Errors are hidden.** `verbose` is false, so process output is thrown
   away, and exit codes are never checked. A failed step prints nothing,
   and later steps run anyway.
7. **`buildWebNatives = true` by default.** On a Mac it clones emsdk and
   runs `emsdk_env.bat` (Windows only) every time.
8. **Hardcoded tool paths.** iOS runs `/usr/local/bin/cmake`. On Apple
   Silicon, Homebrew installs to `/opt/homebrew/bin`. Quorum Studio
   started from the Finder has a minimal `PATH`, so `cmake` by name isn't
   found either.
9. **iOS overwrites FreeType's own `CMakeLists.txt`.**
   `CopyIOSCMakeToFreeType` copies `ios/CMakeLists.txt` over
   `download/freetype/CMakeLists.txt`. That's fine on its own, but it
   breaks building FreeType through CMake (the change below).
10. **Two Quorum library gaps** (library changes, so only noting them):
    - `Process:SetEnvironment` is stored but never used by `Process.java`,
      so everything has to go through command-line arguments.
    - If the program can't be started, `Process.java` catches the
      `IOException` and then calls `watch.sendExceptionToStandardError`
      while `watch` is still null. The result is a NullPointerException
      instead of a clear error. The script below wraps `Run` in
      `check`/`detect` for that reason.

## The new flow

```
AutoBuild (Quorum Studio, Run)
  1. find cmake, check the Xcode command line tools
  2. download + extract FreeType (no JDK downloads on Mac)
  3. cmake -S .. -B ../build/mac        (configures FreeType and our library together)
  4. cmake --build ../build/mac --parallel
       -> build/mac/libGameEngineCPlugins.so  (universal: arm64 + x86_64)
  5. lipo -thin arm64  -> libGameEngineCPluginsArm.so
     lipo -thin x86_64 -> libGameEngineCPlugins.so
  6. copy both into quorum-language/.../Native/Libraries.Game
```

Each step stops the build and prints the failing tool's last output lines.

Why this works:
- FreeType is built **by CMake inside our build**
  (`add_subdirectory(download/freetype)`). `./configure`, `make`,
  `FreeType_Release/` and the hand-built `freetype_Arm.a` all go away.
- `CMAKE_OSX_ARCHITECTURES "arm64;x86_64"` builds **both architectures in
  one build**, FreeType included.
- `Game.java` loads `libGameEngineCPluginsArm.so` on `aarch64` and
  `libGameEngineCPlugins.so` otherwise. `lipo -thin` makes exactly those two
  files, so **no Java change is needed**. (Copying the universal file under
  both names would also work, at twice the size.)

Checked results of the prototype build:
- `lipo -info` gives `x86_64 arm64`.
- The library depends only on `libc++` and `libSystem`, and is ad-hoc
  signed by the linker. The thinned arm64 file passes `codesign -v`.
- Its JNI exports are identical to the shipped
  `libGameEngineCPluginsArm.so`. The shipped file had two functions this
  repository was missing, `FreeTypeStrategy_GetFamily` and
  `FreeTypeStrategy_GetStyleName`. They were rewritten into
  `FreeTypeStrategy.c` from the shipped binary's machine code, and checked
  through JNI on Arial Bold Italic, Times New Roman and Courier New Bold.
- Sizes: arm64 892 KB, x86_64 914 KB. The shipped files are 965 KB and
  1040 KB.

## Change 1: `CMakeLists.txt` (replace the whole file)

This version was built and checked. jpgd is gone because the new
`gdx2d_load` no longer uses it. Delete `jpgd.cpp`, `jpgd.h`, `jpgd_c.cpp`
and `jpgd_c.h` too, unless iOS or another build still lists them.

```cmake
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
```

The same build from Terminal (also handy for CI):

```bash
cmake -S . -B build/mac && cmake --build build/mac --parallel
```

Notes:
- The Windows branch is untested. It should remove the README's
  `freetype.sln` steps too, since FreeType is built by the same CMake run
  (`cmake -S . -B build/win -A x64`, then
  `cmake --build build/win --config Release`).
- `-O2` is now set only on our library. Before, it replaced
  `CMAKE_CXX_FLAGS` globally and dropped CMake's Release flags.

## Change 2: keep iOS from overwriting FreeType's `CMakeLists.txt`

Give iOS its own copy of FreeType, so `download/freetype` stays the
original source:

```quorum
    constant text FREETYPE_IOS_FOLDER = "/freetype-ios"

    // in Main, after the other Deflate calls:
    if buildIOS
        Deflate(FREETYPE_NAME, DOWNLOADS + "/freetype-2.13.0", DOWNLOADS + FREETYPE_IOS_FOLDER)
    end

    // CopyIOSCMakeToFreeType:
    to:SetPath(DOWNLOADS + FREETYPE_IOS_FOLDER + "/" + CMAKE_LISTS)

    // CMakeFreetypeIOS:
    params:Add("../download/freetype-ios/CMakeLists.txt")
```

`ios/CMakeLists.txt` refers to its sources relative to the FreeType folder
it's copied into, so check that its paths still resolve from
`download/freetype-ios`. Also have `CMakeFreetypeIOS` use `FindCMake()`
(below) instead of `/usr/local/bin/cmake`.

**If you already ran an iOS build:** `download/freetype/CMakeLists.txt` is
the iOS file. Delete `download/freetype` and run again so it's
re-extracted.

## Change 3: `Main.quorum` (Mac path)

### 3a. Settings and state (add to the class)

```quorum
    constant text MAC_BUILD_FOLDER = "../build/mac"
    constant text MAC_UNIVERSAL_LIBRARY = "../build/mac/libGameEngineCPlugins.so"
    constant text MAC_ARM_NAME = "libGameEngineCPluginsArm.so"
    constant text MAC_X86_NAME = "libGameEngineCPlugins.so"

    // Where the finished libraries go. Relative to AutoBuild, assuming
    // quorum-natives and quorum-language are in the same folder.
    constant text STANDARD_LIBRARY_GAME_NATIVES = "../../quorum-language/Quorum/Library/Standard/Native/Libraries.Game"
    boolean copyToStandardLibrary = true

    // Set by the ProcessListener actions. Process:Run waits until the
    // process ends and its last output and ProcessStopped have arrived.
    integer lastExit = -1
    Array<text> lastOutput
```

Change the defaults:

```quorum
    boolean buildWebNatives = false   // was true; Windows only today
```

### 3b. `Main`: download only what the platform needs

```quorum
    action Main
        Properties prop
        output "Downloading FreeType"
        Download(FREETYPE, FREETYPE_NAME)
        Deflate(FREETYPE_NAME, DOWNLOADS + "/freetype-2.13.0", DOWNLOADS + "/freetype")

        if prop:IsWindows()
            output "Downloading the Windows JDK"
            Download(WINDOWS_JDK, WINDOWS_JDK_NAME)
            Deflate(WINDOWS_JDK_NAME, DOWNLOADS + "/jdk-11.0.15+10", DOWNLOADS + "/jdk/windows/x64/jdk-11.0.15+10")
            if buildWebNatives
                output "Checking for Emscripten repository..."
                Clone(EMSCRIPTEN_REPO_SOURCE, EMSCRIPTEN_REPO_FOLDER)
                ActivateEmscripten()
            end
        end

        DoPlatformBuild()
        output "Done"
    end
```

### 3c. Replace `DoMac`

```quorum
    /*
        Builds both Mac libraries in one CMake build (FreeType included),
        splits them by architecture, and copies them to the standard library.
    */
    action DoMac
        text cmake = FindCMake()
        if cmake = ""
            output "CMake was not found. Install it (brew install cmake, or CMake.app from cmake.org) and run again."
            return now
        end

        File root
        root:SetPath("..")

        Array<text> parameters
        parameters:Add("-p")
        if not RunStep("Checking the Xcode command line tools", "xcode-select", parameters, root)
            output "Install them with: xcode-select --install"
            return now
        end

        parameters:Empty()
        parameters:Add("-S")
        parameters:Add(".")
        parameters:Add("-B")
        parameters:Add("build/mac")
        parameters:Add("-DCMAKE_BUILD_TYPE=Release")
        parameters:Add("-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64")
        if not RunStep("Configuring (CMake)", cmake, parameters, root)
            return now
        end

        parameters:Empty()
        parameters:Add("--build")
        parameters:Add("build/mac")
        parameters:Add("--parallel")
        if not RunStep("Building FreeType and the natives (first build takes a minute)", cmake, parameters, root)
            return now
        end

        if not Thin("arm64", MAC_ARM_NAME) or not Thin("x86_64", MAC_X86_NAME)
            return now
        end

        if copyToStandardLibrary
            CopyToStandardLibrary(MAC_ARM_NAME)
            CopyToStandardLibrary(MAC_X86_NAME)
        end
        output "Mac natives built: " + MAC_BUILD_FOLDER
    end

    /* Makes a one-architecture copy of the universal library. */
    action Thin(text architecture, text name) returns boolean
        File root
        root:SetPath("..")
        Array<text> parameters
        parameters:Add("build/mac/libGameEngineCPlugins.so")
        parameters:Add("-thin")
        parameters:Add(architecture)
        parameters:Add("-output")
        parameters:Add("build/mac/thin/" + name)

        File thinFolder
        thinFolder:SetPath(MAC_BUILD_FOLDER + "/thin")
        if not thinFolder:Exists()
            thinFolder:CreateDirectories()
        end
        return RunStep("Making the " + architecture + " library", "lipo", parameters, root)
    end

    action CopyToStandardLibrary(text name)
        File from
        from:SetPath(MAC_BUILD_FOLDER + "/thin/" + name)
        File to
        to:SetPath(STANDARD_LIBRARY_GAME_NATIVES + "/" + name)
        if not to:GetParentDirectory():Exists()
            output "Standard library folder not found, skipping the copy: " + to:GetParentDirectory():GetAbsolutePath()
            return now
        end
        if from:Copy(to)
            output "Copied to " + to:GetAbsolutePath()
        else
            output "Could not copy to " + to:GetAbsolutePath()
        end
    end

    /*
        Quorum Studio started from the Finder has a short PATH, so look in
        the usual install places instead of relying on "cmake" by name.
    */
    action FindCMake returns text
        Array<text> places
        places:Add("/opt/homebrew/bin/cmake")
        places:Add("/usr/local/bin/cmake")
        places:Add("/Applications/CMake.app/Contents/bin/cmake")
        i = 0
        repeat while i < places:GetSize()
            File candidate
            candidate:SetAbsolutePath(places:Get(i))
            if candidate:Exists()
                return places:Get(i)
            end
            i = i + 1
        end
        return ""
    end
```

### 3d. One process runner that stops on failure

Replace the four `ProcessListener` actions and add `RunStep`:

```quorum
    /*
        Runs a program and waits for it. Returns true if it exited with 0.
        On failure, prints the last lines it wrote so the cause is visible
        even when verbose is false.
    */
    action RunStep(text description, text program, Array<text> parameters, File folder) returns boolean
        output description
        lastExit = -1
        lastOutput:Empty()

        Process process
        process:SetDirectory(folder)
        process:SetName(program)
        process:SetParameters(parameters)
        process:AddListener(me)
        check
            process:Run()
        detect e
            output "Could not start " + program + ": " + e:GetErrorMessage()
            return false
        end

        if lastExit not= 0
            output description + " failed (exit code " + lastExit + "). Last output:"
            integer start = lastOutput:GetSize() - 40
            if start < 0
                start = 0
            end
            i = start
            repeat while i < lastOutput:GetSize()
                output "    " + lastOutput:Get(i)
                i = i + 1
            end
            return false
        end
        return true
    end

    action ProcessStarted(ProcessEvent event)
        if verbose
            output "Starting " + event:GetName()
        end
    end

    action Error(ProcessEvent event)
        lastOutput:Add(event:GetOutput())
        if verbose
            output event:GetOutput()
        end
    end

    action Output(ProcessEvent event)
        lastOutput:Add(event:GetOutput())
        if verbose
            output event:GetOutput()
        end
    end

    action ProcessStopped(ProcessEvent event)
        lastExit = event:GetExit()
        if verbose
            output "Finished " + event:GetName()
        end
    end
```

`Deflate`, `Clone`, `ActivateEmscripten` and the iOS steps can move to
`RunStep` the same way, so a failed `tar` or `xcodebuild` stops the build
too.

### 3e. Remove

- `ConfigureFreetype`, `CleanFreetype`, `BuildFreetype` and
  `CopyFreeTypeLibrary` from the Mac path (Windows still calls them, until
  Windows moves to the CMake build).
- The `arm` flag.
- `MAC_JDK_ARM`, `MAC_JDK_X86` and their names, along with the
  `download/jdk/mac` folders.

## Change 4: README, Mac section

Replace steps 1-17 with:

```
1. Install CMake 3.16 or newer (brew install cmake, or CMake.app) and the
   Xcode command line tools (xcode-select --install).
2. Install Quorum Studio 4.3 or above.
3. Open AutoBuild in Quorum Studio and run it.

The two libraries are copied to
quorum-language/Quorum/Library/Standard/Native/Libraries.Game
(set copyToStandardLibrary to false to skip that). They are also in
build/mac/thin. Then rebuild the standard library as usual.
```

## Change 5 (optional): image loader tests

`gdx2d_load` was just rewritten (stb_image 2.29, `requestedChannels`, no
jpgd). A small test keeps it from breaking again without needing Java. It
uses `QUORUM_NATIVES_TESTS` in the CMake file above. Put a few images in
`tests/images` (8- and 16-bit PNG, gray, gray-alpha, paletted, baseline and
progressive JPEG, BMP, TGA, plus one truncated PNG), then add
`tests/ImageLoaderTests.c`:

```c
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gdx2d.h"

// Loads every file in a folder with each requested channel count. Files
// whose names start with "bad" must fail; every other file must load with
// the requested format (or a valid one for 0). Exit code 1 on any problem.
int main(int argc, char** argv) {
    int failures = 0;
    DIR* folder = opendir(argv[1]);
    struct dirent* entry;
    if (!folder) return 1;
    while ((entry = readdir(folder)) != NULL) {
        char path[4096];
        FILE* file;
        long size;
        unsigned char* bytes;
        uint32_t channels;
        if (entry->d_name[0] == '.') continue;
        snprintf(path, sizeof(path), "%s/%s", argv[1], entry->d_name);
        file = fopen(path, "rb");
        if (!file) continue;
        fseek(file, 0, SEEK_END); size = ftell(file); fseek(file, 0, SEEK_SET);
        bytes = malloc(size > 0 ? size : 1);
        fread(bytes, 1, size, file); fclose(file);
        for (channels = 0; channels <= 4; channels++) {
            gdx2d_pixmap* map = gdx2d_load(bytes, (uint32_t)size, channels);
            int shouldFail = strncmp(entry->d_name, "bad", 3) == 0;
            int ok = shouldFail ? map == NULL
                : map != NULL && (channels == 0 ? map->format >= 1 && map->format <= 4 : map->format == channels);
            if (!ok) {
                printf("FAIL %s with %u channels: %s\n", entry->d_name, channels,
                       map ? "wrong format" : gdx2d_get_failure_reason());
                failures++;
            }
            if (map) gdx2d_free(map);
        }
        free(bytes);
    }
    closedir(folder);
    return failures == 0 ? 0 : 1;
}
```

Run it:

```bash
cmake -S . -B build/test -DQUORUM_NATIVES_TESTS=ON && cmake --build build/test && ctest --test-dir build/test --output-on-failure
```

## Order of work

1. Change 1 (CMake), then check the Terminal command above on a clean
   clone.
2. Change 3 (Quorum script), then run it from Quorum Studio.
3. Change 2 (iOS folder), then run an iOS build once.
4. Changes 4 and 5.
5. Later: move Windows to the same CMake build, and fix the two `Process`
   library gaps (`SetEnvironment` is ignored, and a NullPointerException
   when a program can't start).
