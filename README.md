# quorum-natives

The C and C++ native libraries for the Quorum programming language, called from Java through JNI: for the game engine, image loading (stb_image), fonts (FreeType), and buffer and matrix helpers; text to speech on the Mac, iOS and Windows; and, on Windows, accessibility (UI Automation). The finished libraries are copied into the standard library in `quorum-language/Quorum/Library/Standard/Native`.

| Platform | File | Where it goes in `Native/` |
|---|---|---|
| Mac, Apple Silicon and Intel (one universal file) | `Mac.so` | `Libraries.Game` |
| Windows, 64-bit: the game natives, accessibility and speech | `Windows.dll` | `Libraries.Game` |
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

AutoBuild installs **CMake**, and the tools for `Windows.dll` (**llvm** for `clang-cl`, **lld**, **xwin** and **ninja**), with Homebrew if they're missing.

**For `Windows.dll`, once:** Microsoft's Windows SDK and C runtime are downloaded with `xwin`, under Microsoft's license, into `download/xwin` (about 1.5 GB; `download/` is ignored by git, and these files must never be committed). AutoBuild can't answer xwin's license question, so either run this once in Terminal, from the root of `quorum-natives`, and accept the license:

```bash
xwin --cache-dir download/xwin-cache --arch x86_64 splat --output download/xwin
```

or set `acceptMicrosoftLicense` to `true` in `Main.quorum`. Until one of these is done, AutoBuild skips Windows and says why.

**Then:**
1. Keep `quorum-natives` and `quorum-language` in the same folder. The finished files are copied to `../quorum-language/Quorum/Library/Standard/Native`.
2. Open the project `AutoBuild` in Quorum Studio and run it. The first run downloads FreeType, the Windows JDK's headers and WIL, and takes a few minutes. Later runs only rebuild what changed.
3. At the end, a summary lists which platforms built and which failed. A failed step stops only its own platform, and prints the last 40 lines of the tool's output.
4. Rebuild the standard library as usual.

**Settings** near the top of `AutoBuild/SourceCode/Main.quorum`:

| Setting | Default | Meaning |
|---|---|---|
| `buildMac` | `true` | Builds the Mac library, `Mac.so`, for Apple Silicon and Intel. |
| `macVersion` | `"11.0"` | The oldest macOS the library loads on. |
| `buildWindows` | `true` | Builds `Windows.dll`: cross-compiled on a Mac with clang-cl, or built on Windows with Visual Studio. |
| `acceptMicrosoftLicense` | `false` | On a Mac: lets AutoBuild run xwin with `--accept-license`, accepting Microsoft's license for the Windows SDK and C runtime. |
| `buildIOS` | `true` | Builds the iOS device and simulator libraries. |
| `iosVersion` | `"14.0"` | The oldest iOS the libraries support. |
| `installVisualStudioBuildTools` | `false` | On Windows only: install the Visual Studio Build Tools, with ATL, with `winget` if they're not found. |
| `buildWebNatives` | `true` | Builds the web module (WebAssembly) with Emscripten, on a Mac or Windows. The first time downloads Emscripten (about 1 GB). |
| `copyToStandardLibrary` | `true` | Copies the results into the standard library. If false, they stay in `build/`. |
| `verbose` | `false` | Prints every tool's output, not only on failure. |

**Testing `Windows.dll`:** it's built on the Mac but can't run there, so test it on Windows before shipping it.

### How it works

Everything is built from the `CMakeLists.txt` in the root folder, one build folder per platform:

| Platform | Folder | How |
|---|---|---|
| Mac | `build/mac` | One build for both `arm64` and `x86_64`, with FreeType built in, giving one universal file, `Mac.so`. `lipo -verify_arch` checks it has both. macOS loads the half that matches the running Java. |
| Windows | `build/windows-clang` | Cross-compiled with `clang-cl` and `lld-link` (`toolchains/clang-cl-xwin.cmake`), using Microsoft's Windows SDK and C runtime from `xwin`, with the C runtime linked in (`/MT`). xwin has no ATL, so the speech code uses the stand-ins in `src/windows/speech/atl` (`QUORUM_ATL_STANDIN`). |
| iOS | `build/ios-device`, `build/ios-simulator` | `CMAKE_SYSTEM_NAME=iOS`. Static libraries with the natives, `src/ios`, and FreeType merged in (`liballnatives.a`). |

The same builds from Terminal, from the root of `quorum-natives`:

```bash
cmake -S . -B build/mac -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 && cmake --build build/mac --parallel
```

```bash
cmake -S . -B build/windows-clang -G Ninja -DCMAKE_TOOLCHAIN_FILE=$PWD/toolchains/clang-cl-xwin.cmake && cmake --build build/windows-clang
```

(After AutoBuild, or xwin and the WIL download, has filled `download/xwin` and `download/wil`.)

```bash
cmake -S . -B build/ios-device -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphoneos -DCMAKE_OSX_ARCHITECTURES=arm64 && cmake --build build/ios-device --parallel
```

For the simulator, use `-DCMAKE_OSX_SYSROOT=iphonesimulator "-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64"` and `build/ios-simulator`.

### Troubleshooting

- **"download/freetype/CMakeLists.txt is the iOS build's copy":** an older iOS build replaced FreeType's own CMakeLists.txt. AutoBuild fixes this itself by extracting FreeType again. By hand, delete `download/freetype` and run AutoBuild.
- **"CMake was not found":** install Homebrew, or `brew install cmake`, and run again. Quorum Studio started from the Finder doesn't see your Terminal `PATH`, so AutoBuild looks in `/opt/homebrew/bin`, `/usr/local/bin` and `CMake.app`.
- **iOS fails at "Checking for the iOS SDK":** install the full Xcode, not just the command line tools.
- **"Windows: skipped. Microsoft's license ... hasn't been accepted":** run the `xwin` command above once, or set `acceptMicrosoftLicense`.
- **"MinGW isn't supported":** an old `build/windows` folder may hold a MinGW build. AutoBuild on a Mac now builds in `build/windows-clang`; delete `build/windows` if you build by hand.

### More information

The plans and history for this build are in the plans repository, `plans/Plans/Natives/`:
- `AutoBuildChanges.md`: why the build was rebuilt this way, and what changed;
- `LinuxNatives.md`: a plan for Linux natives (not built yet);
- `Handoff.md`: where the natives work stands.

## Building on Windows

On Windows, AutoBuild builds **`Windows.dll`** only. The Mac and iOS natives need Apple's compilers and SDKs, which only run on a Mac, so they're skipped (the summary says so).

**Install these once:**
1. **Quorum Studio**, to run AutoBuild.
2. **Visual Studio** (2022 or newer), or just its free **Build Tools**, with the "Desktop development with C++" workload **and** "C++ ATL for latest build tools" (https://visualstudio.microsoft.com/downloads). ATL is optional in the installer, so it has to be ticked. AutoBuild finds them with Microsoft's `vswhere`. Setting `installVisualStudioBuildTools` to `true` in `Main.quorum` makes AutoBuild install the Build Tools, with ATL, itself with `winget`. They're several gigabytes, and Windows asks for permission.

MinGW can't be used: the accessibility code needs Microsoft's WIL, which only builds with Microsoft's compiler or clang.

**CMake** is installed by AutoBuild with `winget` if it isn't already in `C:/Program Files/CMake` or on the `PATH`.

**Then** run AutoBuild, as on a Mac. It downloads FreeType, the Windows JDK's headers and WIL, then builds into `build/windows`, giving `build/windows/Release/Windows.dll`. On Windows the real ATL is used, not the stand-ins.

`Windows.dll` is copied into the standard library's `Native/Libraries.Game`.

By hand, from the root of `quorum-natives`, after AutoBuild has downloaded FreeType and the JDK once:

```
cmake -S . -B build/windows -A x64
cmake --build build/windows --config Release
```

## What's in `Windows.dll`

| Part | Source | Notes |
|---|---|---|
| Game natives | `src/main/cpp` | The same as on the Mac. |
| Accessibility (UI Automation) | `src/windows/accessibility` | Moved from `quorum-language/Plugins/AccessibilityPlugin/Native/Windows/src` (2026). Uses Microsoft's WIL, downloaded into `download/wil`. Each `...Control.cpp` ends with an explicit `template class ControlT<...>;` so optimized builds keep `ControlT`'s members for other files to link to. |
| Speech: SAPI | `src/windows/speech` | The 2019 `SappyJNI` from the Phonemic project, without its .NET leftovers. Windows speech always uses SAPI: the JAWS and NVDA engines were removed in 2026 (screen readers still read Quorum's interface through accessibility). |
| ATL stand-ins | `src/windows/speech/atl` | Only for the clang-cl build on a Mac, which has no ATL: small versions of `CComPtr` and `CString`. A Visual Studio build uses the real ATL. |

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
