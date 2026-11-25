cmake_minimum_required(VERSION 3.16)
project(shogun_linux C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

file(GLOB WASM3_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/third_party/wasm3/source/*.c")

add_executable(shogun src/main.c ${WASM3_SOURCES})

target_include_directories(shogun PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/wasm3/source"
)

target_link_libraries(shogun PRIVATE PkgConfig::SDL2 m)

target_compile_options(shogun PRIVATE -O2)
