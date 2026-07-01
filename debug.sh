#!/bin/bash

cmake -S . -B Build/Debug -D CMAKE_BUILD_TYPE=Debug
cmake --build Build/Debug --config Debug -j
cd Build/Debug/Run

./MyProject
cd ../../..
