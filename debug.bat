@echo off
cls

Rem Build
if not exist Build\Debug\Run mkdir Build\Debug\Run
cd Build\Debug
cmake.exe -S ..\..\ -B . -G"MinGW Makefiles" -D CMAKE_BUILD_TYPE=Debug
cmake --build .

Rem Copy dlls
cd Run
set SFML_BIN="C:\Users\q44\Documents\Libs\SFML-3.0.0\bin"
echo n | copy /-y %SFML_BIN%\sfml-system-d-3.dll .
echo n | copy /-y %SFML_BIN%\sfml-graphics-d-3.dll .
echo n | copy /-y %SFML_BIN%\sfml-window-d-3.dll .

Rem Lauch
MyProject.exe
cd ..\..\..
