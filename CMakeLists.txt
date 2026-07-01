include(./.env.cmake OPTIONAL RESULT_VARIABLE LOCAL_ENV)
message(STATUS "Local .env.cmake: ${LOCAL_ENV}")

cmake_minimum_required(VERSION 3.21)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Run)

project(MyProject VERSION 1.0)

find_package(SFML 3 REQUIRED COMPONENTS System Window Graphics)
find_package(OpenCL REQUIRED)

# ===== My utils ================================================== #

file(GLOB_RECURSE UTILS_SOURCES ${UTILS_PATH}/src/*.cpp)

# Remove TIFF from sources
list(REMOVE_ITEM UTILS_SOURCES ${UTILS_PATH}/src/loadTif.cpp)
list(REMOVE_ITEM UTILS_SOURCES ${UTILS_PATH}/src/saveTif.cpp)

add_library(Utils STATIC ${UTILS_SOURCES})

target_include_directories(Utils PUBLIC ${UTILS_PATH}/include)

# ===== Main ====================================================== #

file(GLOB_RECURSE SOURCES ${PROJECT_SOURCE_DIR}/src/*.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})

target_compile_options(${PROJECT_NAME} PRIVATE -Wall)

target_include_directories(${PROJECT_NAME} PRIVATE
  ${UTILS_PATH}/include
)

target_precompile_headers(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/src/pch.hpp)

target_link_libraries(${PROJECT_NAME} PRIVATE
  SFML::System
  SFML::Window
  SFML::Graphics
  OpenCL::OpenCL
  Utils
)

target_compile_definitions(${PROJECT_NAME} PRIVATE CL_TARGET_OPENCL_VERSION=300)

