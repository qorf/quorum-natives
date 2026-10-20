# Linux Natives: what it would take

Notes for later (2026-10-20). Nothing here has been built or changed yet.

## Summary

The natives themselves are the easy part. The code is plain C and C++
that FreeType and stb_image both support on Linux. The root
`CMakeLists.txt` needs a small Linux branch, and AutoBuild can
cross-compile from a Mac with Zig.

The real work is outside this repository:
- `Game.java` gives Linux the same library names as Mac.
- The standard library ships no Linux LWJGL natives.
- Accessibility has no Linux library.

So the first decision is whether Quorum supports Linux as a whole. If it
does, adding the build is a few hours of work.

## What was checked

| Item | Finding |
|---|---|
| Native sources | Portable C/C++. No Mac-only or Windows-only calls in `src/main/cpp`. |
| `Game.java` (Plugins/QuorumPlugins/.../Game/Game.java:36) | `if (os.contains("Mac OS X") \|\| os.contains("Linux"))`: Linux takes the Mac branch and loads `jni/libGameEngineCPluginsArm.so` on `aarch64` (:54-55), otherwise `jni/libGameEngineCPlugins.so`. These are the Mac file names. |
| LWJGL natives in `AutoBuild/Run/libraries` | 24 `natives-macos-arm64`, 24 `natives-macos`, 22 `natives-windows` jars. **No `natives-linux`.** The standard library ships none either. |
| `Native/Libraries.Interface` | `AccessibilityManagerWindows64.dll` and `libaccesskit_jni.dylib`. **No Linux library.** |
| `Native/Libraries.Sound.Speech` | Has `libLinuxSpeakJNI.so` and `libLinuxSpeakJNI64.so`, so Linux speech existed at some point. They're probably old, so check whether they still load. |

## Work list

### 1. Pick Linux library names, and change `Game.java` (standard library)

A Linux library can't share the Mac names, since the `jni` folder holds
every platform's files. Suggested names:

| Platform | File |
|---|---|
| Linux x86_64 | `libGameEngineCPluginsLinux64.so` |
| Linux arm64 | `libGameEngineCPluginsLinuxArm64.so` |

A sketch of the `Game.java` change. It's a library change, so it needs
review first:

```java
String arch = System.getProperty("os.arch");
boolean arm = arch.contains("aarch64") || arch.contains("arm64");
if (os.contains("Linux")) {
    nativeFile = runLocation + (arm ? "/jni/libGameEngineCPluginsLinuxArm64.so"
                                    : "/jni/libGameEngineCPluginsLinux64.so");
} else { // Mac
    nativeFile = runLocation + (arm ? "/jni/libGameEngineCPluginsArm.so"
                                    : "/jni/libGameEngineCPlugins.so");
}
```

The Android check (`java.runtime.name` contains "Android Runtime") has to
stay ahead of this, because Android also reports `Linux`.

Whatever copies `Native/Libraries.Game` into a project's `jni` folder
(the compiler or Quorum Studio) has to include the new files too.

### 2. Root `CMakeLists.txt`: a Linux branch

Add this next to the existing Mac and Windows branches:

```cmake
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(QUORUM_JDK "${CMAKE_SOURCE_DIR}/download/jdk/linux/x64/jdk-11.0.15+10" CACHE PATH "JDK whose headers are used")
    set(JNI_PLATFORM linux)
```

Then add output names and runtime linking:

```cmake
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|arm64")
        set_target_properties(GameEngineCPlugins PROPERTIES PREFIX "lib" OUTPUT_NAME "GameEngineCPluginsLinuxArm64")
    else()
        set_target_properties(GameEngineCPlugins PROPERTIES PREFIX "lib" OUTPUT_NAME "GameEngineCPluginsLinux64")
    endif()
    # Don't depend on the user's libstdc++ version.
    target_link_options(GameEngineCPlugins PRIVATE -static-libgcc -static-libstdc++)
```

Notes:
- `jni_md.h` for Linux only differs from Darwin's in small ways, but use
  the Linux JDK's headers to be safe. Only the `include` folder is needed.
- `CMAKE_POSITION_INDEPENDENT_CODE` is already on, which a Linux `.so`
  with FreeType linked in statically needs.

### 3. Cross-compiling from a Mac: Zig

| Option | Pros | Cons |
|---|---|---|
| **Zig** (`zig cc`) | One download that AutoBuild can fetch like FreeType. Targets x86_64 and aarch64. **Lets you pick the glibc version.** | Its CMake support works through small wrapper scripts. |
| Docker | Builds on real Linux. | Needs Docker Desktop and a running daemon. |
| Homebrew cross-toolchains (`messense/macos-cross-toolchains`) | GCC, like MinGW. | A third-party tap. The glibc version is fixed by the toolchain. |
| Build on Linux | No cross-compiling. | Not a one-step Mac build. |

**glibc matters.** A `.so` linked against a new glibc won't load on older
distributions. Targeting glibc 2.17 covers nearly every distribution still
in use. With Zig: `--target=x86_64-linux-gnu.2.17` and
`--target=aarch64-linux-gnu.2.17`.

A sketch of `toolchains/zig-linux.cmake`. `ZIG_TARGET` is passed with `-D`.
CMake wants a single compiler path, so two tiny wrapper scripts call Zig:

```cmake
# cmake -S . -B build/linux-x64 -DCMAKE_TOOLCHAIN_FILE=toolchains/zig-linux.cmake -DZIG_TARGET=x86_64-linux-gnu.2.17
set(CMAKE_SYSTEM_NAME Linux)
if (ZIG_TARGET MATCHES "^aarch64")
    set(CMAKE_SYSTEM_PROCESSOR aarch64)
else()
    set(CMAKE_SYSTEM_PROCESSOR x86_64)
endif()
set(CMAKE_C_COMPILER "${CMAKE_CURRENT_LIST_DIR}/zig-cc")
set(CMAKE_CXX_COMPILER "${CMAKE_CURRENT_LIST_DIR}/zig-c++")
set(CMAKE_C_COMPILER_TARGET ${ZIG_TARGET})
set(CMAKE_CXX_COMPILER_TARGET ${ZIG_TARGET})
set(CMAKE_AR "${CMAKE_CURRENT_LIST_DIR}/zig-ar")
set(CMAKE_RANLIB "${CMAKE_CURRENT_LIST_DIR}/zig-ranlib")
```

`toolchains/zig-cc` (and the same for `zig-c++`, `zig-ar`, `zig-ranlib`):

```sh
#!/bin/sh
exec "${ZIG:-zig}" cc "$@"
```

Check when building: whether Zig's static libc++ makes
`-static-libstdc++` unnecessary (Zig links its own libc++ statically by
default).

### 4. AutoBuild (`Main.quorum`)

These follow the patterns already in the script (`FindTool`,
`BuildWithCMake`, `CopyToStandardLibrary`, `summary`):
- A `boolean buildLinux = true` flag, and a `DoLinuxOnMac` action called
  from `DoMacHost`.
- **Zig:** `FindTool("zig", "zig")` installs it with Homebrew. Or
  download the Zig tarball from ziglang.org into `download/` like
  FreeType, which pins the version and needs no Homebrew.
- **JDK headers:** download the Linux JDK like the Windows one, into
  `download/jdk/linux/x64/jdk-11.0.15+10`. Adoptium has
  `OpenJDK11U-jdk_x64_linux_hotspot_11.0.15_10.tar.gz`.
- Two `BuildWithCMake` calls: `build/linux-x64` with
  `-DZIG_TARGET=x86_64-linux-gnu.2.17`, and `build/linux-arm64` with
  `-DZIG_TARGET=aarch64-linux-gnu.2.17`. Both also get
  `-DCMAKE_TOOLCHAIN_FILE=<absolute path>/toolchains/zig-linux.cmake`.
- Copy both `.so` files to `Native/Libraries.Game`.

### 5. The rest of Quorum (standard library and Studio)

- **LWJGL Linux natives:** add `natives-linux` and `natives-linux-arm64`
  for every LWJGL module in use (core, GLFW, OpenGL, OpenAL, STB, NFD,
  shaderc, VMA, Assimp, and so on), at the same LWJGL version as the Mac
  and Windows jars. Note there's a mix of 3.3.1 and 3.4.1 jars today.
- **Accessibility:** AccessKit supports Linux through AT-SPI, so a
  `libaccesskit_jni.so` built for Linux may be possible. This needs a
  decision, because accessibility is central to Quorum.
- **Speech:** check whether `libLinuxSpeakJNI*.so` still load and work.
  Rebuild them, or use speech-dispatcher, if not.
- **Vulkan:** works on Linux through LWJGL with the distribution's drivers.
  It needs testing, especially on arm64 boards.

### 6. Testing

Like Windows, Linux libraries can be built on a Mac but must be run on
Linux:
- **arm64:** a Linux VM on Apple Silicon (UTM, or Parallels with
  Ubuntu ARM64). Fast, and it runs arm64 natively.
- **x86_64:** a CI runner (GitHub Actions `ubuntu-latest`) or a spare
  PC. Emulating x86_64 in UTM works but is slow.
- **Quick checks on the Mac:**
  - `file libGameEngineCPluginsLinux64.so` should show an ELF 64-bit
    shared object.
  - `nm -D --defined-only` on the library should show the same `Java_`
    exports as the Mac library. Homebrew's `binutils` provides an
    ELF-capable `nm`.
  - `objdump -T` on the library should show no `GLIBC_` versions newer
    than the target (2.17).

## Suggested order

1. Decide on Linux support for Quorum as a whole, including LWJGL Linux
   natives and accessibility.
2. Choose library names and make the `Game.java` change (item 1).
3. Add the CMake Linux branch and the Zig toolchain, and build both
   architectures by hand from a Mac (items 2 and 3).
4. Test in an arm64 Linux VM with a small Quorum game.
5. Add the build to AutoBuild (item 4).
