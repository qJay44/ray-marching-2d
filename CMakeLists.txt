include(./.env.cmake OPTIONAL RESULT_VARIABLE LOCAL_ENV)
message(STATUS "Local .env.cmake: ${LOCAL_ENV}")

cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Run)

project(MyProject VERSION 1.0)

find_package(SFML 3 REQUIRED COMPONENTS System Window Graphics PATHS ${SFML_PATH})

file(GLOB_RECURSE SOURCES
  ${PROJECT_SOURCE_DIR}/src/*.cpp
  ${UTILS_PATH}/src/*.cpp
)

list(REMOVE_ITEM SOURCES ${UTILS_PATH}/src/loadTif.cpp)
list(REMOVE_ITEM SOURCES ${UTILS_PATH}/src/saveTif.cpp)

add_executable(${PROJECT_NAME} ${SOURCES})

target_compile_options(${PROJECT_NAME} PRIVATE -Wall)

target_include_directories(${PROJECT_NAME} PUBLIC
  ${OCL_PATH}/include
  ${UTILS_PATH}/include
)

target_precompile_headers(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/src/pch.hpp)

target_link_directories(${PROJECT_NAME} PUBLIC ${OCL_PATH}/lib/x86_64)
target_link_libraries(${PROJECT_NAME} PRIVATE SFML::System SFML::Window SFML::Graphics OpenCL)
