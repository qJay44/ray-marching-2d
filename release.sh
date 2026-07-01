#!/bin/bash

cmake -S . -B Build/Release -D CMAKE_EXPORT_COMPILE_COMMANDS=ON -D CMAKE_BUILD_TYPE=Release
cmake --build Build/Release --config Release -j
ln -s Build/Release/compile_commands.json .
cd Build/Release/Run

./MyProject
cd ../../..
