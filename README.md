# quorum-natives

The C and C++ native libraries for the Quorum programming language's game engine: image loading (stb_image), fonts (FreeType), and buffer and matrix helpers, called from Java through JNI. The finished libraries are copied into the standard library in `quorum-language/Quorum/Library/Standard/Native`.

| Platform | File | Where it goes in `Native/` |
|---|---|---|
| Mac, Apple Silicon | `libGameEngineCPluginsArm.so` | `Libraries.Game` |
| Mac, Intel | `libGameEngineCPlugins.so` | `Libraries.Game` |
| Windows, 64-bit | `libGameEngineCPlugins64.dll` | `Libraries.Game` |
| iPhone and iPad | `ios.a` | `mobile` |
| iOS Simulator | `iosSimulator.a` | `mobile` |

## Building everything on a Mac (recommended)

One run of the AutoBuild project builds the Mac, Windows, iOS and web natives and copies them into the standard library.

**Install these once:**
1. **Homebrew** (https://brew.sh). Its installer asks for your password, so it can't be installed for you:

   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   ```
2. **The Xcode command line tools**, with `xcode-select --install`. For iOS you need the full **Xcode** from the App Store, which includes the iOS SDK.
3. **Quorum Studio 4.3 or above** (https://quorumlanguage.com/download.html). Its JDK provides the JNI headers for Mac and iOS.

AutoBuild installs **CMake** and **MinGW-w64** (the Windows cross-compiler) with Homebrew if they're missing.

**Then:**
1. Keep `quorum-natives` and `quorum-language` in the same folder. The finished files are copied to `../quorum-language/Quorum/Library/Standard/Native`.
2. Open the project `AutoBuild` in Quorum Studio and run it. The first run downloads FreeType and the Windows JDK's headers, and takes a few minutes. Later runs only rebuild what changed.
3. At the end, a summary lists which platforms built and which failed. A failed step stops only its own platform, and prints the last 40 lines of the tool's output.
4. Rebuild the standard library as usual.

**Settings** near the top of `AutoBuild/SourceCode/Main.quorum`:

| Setting | Default | Meaning |
|---|---|---|
| `buildMac` | `true` | Builds the Mac libraries (Apple Silicon and Intel). |
| `buildWindows` | `true` | Builds the Windows DLL: cross-compiled on a Mac, or built directly on Windows. |
| `buildIOS` | `true` | Builds the iOS device and simulator libraries. |
| `iosVersion` | `"14.0"` | The oldest iOS the libraries support. |
| `installVisualStudioBuildTools` | `false` | On Windows only: install the Visual Studio Build Tools with `winget` if no compiler is found. |
| `buildWebNatives` | `true` | Builds the web module (WebAssembly) with Emscripten, on a Mac or Windows. The first time downloads Emscripten (about 1 GB). |
| `copyToStandardLibrary` | `true` | Copies the results into the standard library. If false, they stay in `build/`. |
| `verbose` | `false` | Prints every tool's output, not only on failure. |

**Testing the Windows DLL:** it's built on the Mac but can't run there, so test it on Windows before shipping it.

### How it works

Everything is built from the `CMakeLists.txt` in the root folder, one build folder per platform:

| Platform | Folder | How |
|---|---|---|
| Mac | `build/mac` | One build for both `arm64` and `x86_64`, with FreeType built in. `lipo -thin` then splits it into the two files `Game.java` loads (`build/mac/thin`). |
| Windows | `build/windows` | Cross-compiled with MinGW-w64 (`toolchains/mingw-w64.cmake`), with the GCC runtime linked in. |
| iOS | `build/ios-device`, `build/ios-simulator` | `CMAKE_SYSTEM_NAME=iOS`. Static libraries with the natives, `src/ios`, and FreeType merged in (`liballnatives.a`). |

The same builds from Terminal, from the root of `quorum-natives`:

```bash
cmake -S . -B build/mac && cmake --build build/mac --parallel
```

```bash
cmake -S . -B build/windows -DCMAKE_TOOLCHAIN_FILE=$PWD/toolchains/mingw-w64.cmake && cmake --build build/windows --parallel
```

```bash
cmake -S . -B build/ios-device -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphoneos -DCMAKE_OSX_ARCHITECTURES=arm64 && cmake --build build/ios-device --parallel
```

For the simulator, use `-DCMAKE_OSX_SYSROOT=iphonesimulator "-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64"` and `build/ios-simulator`.

### Troubleshooting

- **"download/freetype/CMakeLists.txt is the iOS build's copy":** an older iOS build replaced FreeType's own CMakeLists.txt. AutoBuild fixes this itself by extracting FreeType again. By hand, delete `download/freetype` and run AutoBuild.
- **"CMake was not found":** install Homebrew, or `brew install cmake`, and run again. Quorum Studio started from the Finder doesn't see your Terminal `PATH`, so AutoBuild looks in `/opt/homebrew/bin`, `/usr/local/bin` and `CMake.app`.
- **iOS fails at "Checking for the iOS SDK":** install the full Xcode, not just the command line tools.

### More information

The plans and history for this build are in the plans repository, `plans/Plans/Natives/`:
- `AutoBuildChanges.md`: why the build was rebuilt this way, and what changed;
- `LinuxNatives.md`: a plan for Linux natives (not built yet);
- `Handoff.md`: where the natives work stands.

## Building on Windows

On Windows, AutoBuild builds the **Windows DLL** only. The Mac and iOS natives need Apple's compilers and SDKs, which only run on a Mac, so they're skipped (the summary says so).

**Install these once:**
1. **Quorum Studio**, to run AutoBuild.
2. **A C++ compiler**, either:
   - **Visual Studio** (2019 or newer), or just its free **Build Tools**, with the "Desktop development with C++" workload (https://visualstudio.microsoft.com/downloads). AutoBuild finds it with Microsoft's `vswhere`. Setting `installVisualStudioBuildTools` to `true` in `Main.quorum` makes AutoBuild install the Build Tools itself with `winget`. They're several gigabytes, and Windows asks for permission; or
   - **MinGW**, from MSYS2 (https://www.msys2.org). In the MSYS2 terminal: `pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make`. AutoBuild looks in `C:/msys64/ucrt64/bin`, `C:/msys64/mingw64/bin`, `C:/mingw64/bin` and Chocolatey's MinGW folder.

   Visual Studio is used if both are installed.

**CMake** is installed by AutoBuild with `winget` if it isn't already in `C:/Program Files/CMake` or on the `PATH`.

**Then** run AutoBuild, as on a Mac. It downloads FreeType and the Windows JDK's headers, then builds:
- with Visual Studio, into `build/windows`, giving `build/windows/Release/libGameEngineCPlugins64.dll`;
- with MinGW, into `build/windows-mingw`.

The DLL is copied into the standard library's `Native/Libraries.Game`.

By hand, from the root of `quorum-natives`, after AutoBuild has downloaded FreeType and the JDK once:

```
cmake -S . -B build/windows -A x64
cmake --build build/windows --config Release
```

## The web module (WebAssembly)

Quorum games in the browser use FreeType through a WebAssembly module: `load.js`, `load.wasm`, and `load.data` (the fonts in `src/web/fonts`, mounted as `/fonts`). The standard library's `Plugins/javascript/Libraries/Game/Graphics/Fonts/FreeTypeStrategy.js` calls its functions.

**AutoBuild builds it on a Mac or on Windows** (`buildWebNatives`, on by default), with the same `CMakeLists.txt` and FreeType as the desktop natives:
1. **Emscripten's SDK** is downloaded into `download/emsdk` (from GitHub, as a zip), and installed and activated the first time. That's about 1 GB, and later runs reuse it.
   - **No Python needs to be installed.** emsdk's installer is a Python program, and on Mac and Windows emsdk installs its own Python. So AutoBuild downloads that same Python first, using the version named in emsdk's launcher and the archive named in its manifest, into `download/emsdk-python-<version>`. It then starts the install with that Python. After that, emsdk uses the Python it installed.
2. **On Windows,** Ninja (the build tool CMake uses with Emscripten there) is downloaded into `download/ninja.exe`.
3. **The build:** Emscripten's tools need its environment variables, so AutoBuild writes a small script, `build/web-build.sh` or `build/web-build.bat`. The script sets up the environment and runs `emcmake cmake -S . -B build/web` and `cmake --build build/web`.
4. **The result:** `load.js`, `load.wasm`, and `load.data` are copied from `build/web` to `quorum-language/Quorum/Library/Standard/Plugins/javascript`.

**What it exports:** every function `FreeTypeStrategy.js` uses, including `loadSDFCharC`. The module the standard library shipped before 2026-10-22 was built without it.

**Fonts:** `src/web/fonts` holds the Fira fonts the standard library lists in `FontManager` (`FiraSans-*`, `FiraMono-*`), plus Lato. They're all under the SIL Open Font License (`OFL-Fira.txt`, `OFL.txt`). Add a font there to make it loadable as `fonts/<name>.otf` or `.ttf`.

**By hand,** with Emscripten's environment set up (`source download/emsdk/emsdk_env.sh` on a Mac, or `download\emsdk\emsdk_env.bat` on Windows), from the root of `quorum-natives`:

```bash
emcmake cmake -S . -B build/web -DCMAKE_BUILD_TYPE=Release && cmake --build build/web
```

On Windows, add `-G Ninja` (with `ninja` on the `PATH`, or `-DCMAKE_MAKE_PROGRAM=` pointing at it).
