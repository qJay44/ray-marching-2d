@echo off
cls

Rem Build
if not exist Build\Release\Run mkdir Build\Release\Run
cd Build\Release
cmake.exe -S ..\..\ -B . -G"MinGW Makefiles" -D CMAKE_EXPORT_COMPILE_COMMANDS=ON -D CMAKE_BUILD_TYPE=Release
cmake --build .
move /y compile_commands.json ..\compile_commands.json

Rem Copy dlls
cd Run
set SFML_BIN="C:\Users\q44\Documents\Libs\SFML-3.0.0\bin"
echo n | copy /-y %SFML_BIN%\sfml-system-3.dll .
echo n | copy /-y %SFML_BIN%\sfml-graphics-3.dll .
echo n | copy /-y %SFML_BIN%\sfml-window-3.dll .

Rem Lauch
MyProject.exe
cd ..\..\..
