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

One run of the AutoBuild project builds the Mac, Windows and iOS natives and copies them into the standard library.

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
| `buildWindows` | `true` | Cross-compiles the Windows DLL. |
| `buildIOS` | `true` | Builds the iOS device and simulator libraries. |
| `iosVersion` | `"14.0"` | The oldest iOS the libraries support. |
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

- `AutoBuildChanges.md`: why the build was rebuilt this way, and what changed.
- `LinuxNatives.md`: a plan for Linux natives (not built yet).

## Building the natives on Windows

The Windows DLL is normally cross-compiled on a Mac (above). To build it on Windows itself with Visual Studio instead:
1. Run the AutoBuild Quorum project, if you haven't already. This downloads and unzips the FreeType source and the Windows JDK into the "download" folder.
2. Install CMake and Visual Studio (2019 or newer, with "Desktop development with C++").
3. From a Developer Command Prompt in the root of `quorum-natives`, run:

```
cmake -S . -B build/win -A x64
cmake --build build/win --config Release
```

The root `CMakeLists.txt` builds FreeType as part of the build, so the older steps of building `freetype.sln` separately aren't needed. This route hasn't been tested yet; AutoBuild on a Mac uses the MinGW route instead.

## Building the Web Assembly Module (.WASM) for web browsers:
Follow these steps to build the natives for use in web browsers. 
These instructions assume you're working on Windows. The process is similar but slightly different on Unix platforms.
For more details on running Emscripten on other platforms, or if you have problems during installation, see their getting started documentation: https://emscripten.org/docs/getting_started/downloads.html#sdk-download-and-install
1. On Windows, Emscripten requires Python 3.6 or newer. You can download a version from here: https://www.python.org/downloads/
2. Run the AutoBuild Quorum project, if you haven't already. This will clone the Emscripten SDK.
3. In a terminal, navigate to "download/emsdk". Then, use the following console commands to update and activate the latest toolset:

git pull
emsdk install latest
emsdk activate latest
emsdk_env.bat

4. You may close the terminal after running the above commands.
5. Using your file browser, navigate to "download/emsdk" and double-click "emcmdprompt.bat". This will open a terminal with paths automatically set to use Emscripten SDK tools.
6. In the terminal, navigate to "download/freetype" using this command: cd ../freetype
7. Use the following console commands in the opened prompt to make a new "build" directory, go into the directory, and build an Emscripten-compatible version of Freetype as a static library:

mkdir build
cd build
emcmake cmake ..
emmake make
emmake make install

8. When you're finished with the above commands, you should have a file named "libfreetype.a" in "download/freetype/build".
9. Using the Emscripten-enabled terminal, navigate to "src/web".
10. Use the following command to compile the web assembly:

em++ FreeTypeStrategy.cpp ../../download/freetype/build/libfreetype.a -I ../../download/freetype/include -std=c++11 -O3 -s WASM=1 -s ASSERTIONS=1 -s LLD_REPORT_UNDEFINED --preload-file fonts -o load.js -s EXPORTED_FUNCTIONS=_main,_malloc,_free,_InitializeFreeTypeC,_LoadFontC,_IsFontAvailableC,_SetSizeC,_SetAngleC,_DisposeC,_GetKerningC,_GetUnderlinePositionC,_GetUnderlineThicknessC,_GetLineHeightC,_GetMaximumAscentC,_GetMaximumDescentC,_loadCharC,_loadSDFCharC,_GetLastError,_getBitmapC,_getBitmapDataC -s EXPORTED_RUNTIME_METHODS=cwrap,getValue -s SAFE_HEAP=1

11. When the command finishes, you should have 3 new files in "src/web": "load.data", "load.js", and "load.wasm". These three files are the finished output. If you successfully built these three, you're done!

