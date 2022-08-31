# quorum-natives

This is an experimental repository to create native libraries for the Quorum programming language. It is experimental in that it currently can build the natives only on Mac and these binaries are not included in the official Quorum build. The target for release of these binaries, including integration into the language, is targeted for Quorum 11 or 12.

To build Quorum-natives, follow these procedures:

1. First download CMake: https://cmake.org/download/
2. Add CMake to the path. Instructions can typically be found from the menu item Tools -> How to Install for Command Line Usage. On Mac, this might look like:

sudo "/Applications/CMake.app/Contents/bin/cmake-gui" --install

3. Install Quorum Studio 4.3 or above: https://quorumlanguage.com/download.html
4. Open the project "AutoBuild" in Quorum Studio and run it. This will download dependencies, including various JDKs and Freetype, and then compile freetype. The process can take some time on first run, like 5 or 10 minutes, depending on the machine. Subsequent builds are smart enough to skip most of the lengthy work.
5. Download and install VSCode: https://code.visualstudio.com
6. Open the root folder of quorum-natives
7. Install the CMake plugin for VSCode. This will pop up automatically for install when opening CMakeLists.txt.
8. Select Build All. This will create a file called libGameEngineCPlugins.so in the root folder. 
