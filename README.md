# quorum-natives

This is an experimental repository to create native libraries for the Quorum programming language. It is experimental in that it currently can build the natives only on Mac and these binaries are not included in the official Quorum build. The target for release of these binaries, including integration into the language, is targeted for Quorum 11 or 12.

## Mac
To build Quorum-natives, follow these procedures:

1. First download CMake: https://cmake.org/download/
2. Add CMake to the path. Instructions can typically be found from the menu item Tools -> How to Install for Command Line Usage. On Mac, this might look like:

sudo "/Applications/CMake.app/Contents/bin/cmake-gui" --install

3.	Install Quorum Studio 4.3 or above: https://quorumlanguage.com/download.html
4.	Open the project "AutoBuild" in Quorum Studio and run it. This will download dependencies, including various JDKs and Freetype, and then compile freetype. The process can take some time on first run, like 5 or 10 minutes, depending on the machine. Subsequent builds are smart enough to skip most of the lengthy work. This will build freetype only in x86_64 mode only, for reasons that are not clear. 
5.	The compiled freetype should show up in FreeType_Release/freetype_x86_64.a. 
6.	Now run the following command from the console in the download/freetype folder on an ARM processor. I am sure there is a workaround for this, but this is the current procedure

./configure --without-zlib --without-png --without-bzip2 -target arm64-apple-macos

7.	Copy the file at objs/.libs/libfreetype.a to FreeType_Release/freetype_Arm.a in the freetype folder in downloads
8.	Double check the architecture of both files is correct:
lipo -info FreeType_Release/freetype_Arm.a
9.	Download and install VSCode: https://code.visualstudio.com
10.	Open the root folder of quorum-natives
11.	Install the CMake plugin for VSCode. This will pop up automatically for install when opening CMakeLists.txt.
12.	On line 2, call Set(ARCH OFF) which will put the files in x86_64 mode for building.
13.	Select Build All. This will create a file called libGameEngineCPlugins.so in the root folder.
14.	Move the compiled file to the name libGameEngineCPlugins.so.
15.	Change the architecture to ON and then move that file to libGameEngineCPluginsArm.so
16.	Finally, move both files to quorum-language/Quorum/Libraries/Standard/Native/Libraries.Game
17.	Re-run the standard library and do a standard library override.

You can also combine them with commands like lipo libGameEngineCPlugins.so libGameEngineCPluginsArm.so -create -output Mac.so

## iPhone and the iPhone Simulator
Compiling for iPhone is generally simpler, as the auto-build script is a bit smarter. Essentially, you set a flag and run the program. 

1.	Open AutoBuild
2.	Set buildIOS to true
3.	Run the program. The files are under ios/releases. Both files need to be moved to the standard library under quorum-language/Quorum/Libraries/Standard/Native/mobile


## Building the Freetype Library for Windows
Follow these steps to build Freetype as a static library on Windows. You'll need this as a dependency for building libGameEngineCPlugins64.dll.
1. Run the AutoBuild Quorum project, if you haven't already. This will automatically download and unzip the Freetype source to the "download" folder.
2. Open the Visual Studio solution project "freetype.sln" located at "download/freetype/builds/windows/vc2010". (These steps have been tested with Visual Studio 2019, though other versions should work as well.)
3. In the toolbar, change the configuration dropdown to "Release Static" and the platform dropdown to "x64". (By default, these most likely appear as "Debug" and "Win32", respectively.)
4. Build the solution by using "Build" > "Build Solution" from the menu, or use the hotkey Ctrl + Shift + B. (The build will produce lots of warnings. This is normal.)
5. The solution will output "freetype.lib" to "download/freetype/objs", as well as an identical copy in "download/freetype/objs/x64/Release Static".
6. With Freetype compiled, you can now finish following the steps above to build the natives for Windows. 
7. If using Visual Studio for your build tools in VSCode, make sure the release variant selected in the blue bar at the bottom of the screen is "Release" and not "Debug" when building with CMake.
8. Build the library using CMake in VS, mirroring the approach for Mac above.


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

