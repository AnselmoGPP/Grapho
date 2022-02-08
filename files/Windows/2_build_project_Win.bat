@ECHO OFF
ECHO Building project...
ECHO Current directory: 
CD

RMDIR /S /Q ..\..\_BUILD\projects
MKDIR ..\..\_BUILD
MKDIR ..\..\_BUILD\projects
CD ..\..\_BUILD\projects

"C:\Program Files\CMake\bin\cmake" -G"Visual Studio 17 2022" ..\..\projects
PAUSE

REM The next step is to build the binaries:
REM 	- Open MVS and set the project you want as "startup project" (proyecto de inicio).
REM 	- Compile with MVS: first, compile GLFW static library; then, the project you want. 
REM 	- Compile both in release mode because this program is linking (in his CMakeLists.txt) the GLFW static library compiled in release mode.
REM 	- Put the correct shader's directory names when creating the shading program
