cmake_minimum_required(VERSION 3.16)
project(shogun_refactor C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

add_executable(shogun
    src/main.c
    src/app.c
    src/art.c
    src/ai.c
    src/board.c
    src/config.c
    src/effects.c
    src/font.c
    src/game.c
    src/input.c
    src/layout.c
    src/menu.c
    src/render.c
    src/rules.c
)

target_include_directories(shogun PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(shogun PRIVATE PkgConfig::SDL2 m)
target_compile_options(shogun PRIVATE -O2 -Wall -Wextra)

enable_testing()
add_executable(logic_test
    tests/logic_test.c
    src/game.c
    src/board.c
    src/rules.c
    src/ai.c
    src/layout.c
)
target_include_directories(logic_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(logic_test PRIVATE m)
add_test(NAME logic COMMAND logic_test)
