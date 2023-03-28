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
