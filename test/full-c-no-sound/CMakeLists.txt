cmake_minimum_required(VERSION 3.16)
project(shogun_linux C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

add_executable(shogun
    src/main.c
    src/game.c
    src/wasm-rt-impl.c
    src/wasm-rt-mem-impl.c
)

target_include_directories(shogun PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

target_link_libraries(shogun PRIVATE PkgConfig::SDL2 m)

target_compile_options(shogun PRIVATE -O2)
