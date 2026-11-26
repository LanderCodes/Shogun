cmake_minimum_required(VERSION 3.16)
project(shogun C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

set(CORE_SOURCES
    src/core/ai.c
    src/core/board.c
    src/core/config.c
    src/core/game.c
    src/core/rules.c
)
set(RENDER_SOURCES
    src/render/art.c
    src/render/effects.c
    src/render/font.c
    src/render/glyph.c
    src/render/layout.c
    src/render/render.c
)
set(UI_SOURCES
    src/ui/app.c
    src/ui/input.c
    src/ui/menu.c
)
set(ASSET_SOURCES
    src/assets/assets.h
)

add_executable(shogun src/ui/main.c ${CORE_SOURCES} ${RENDER_SOURCES} ${UI_SOURCES})
target_include_directories(shogun PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(shogun PRIVATE PkgConfig::SDL2 m)
target_compile_options(shogun PRIVATE -O2 -Wall -Wextra)

enable_testing()

add_executable(logic_test
    tests/logic_test.c
    src/render/glyph.c
    src/render/layout.c
    ${CORE_SOURCES}
    src/ui/menu.c
)
target_include_directories(logic_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(logic_test PRIVATE m)
target_compile_options(logic_test PRIVATE -UNDEBUG -Wall -Wextra)
add_test(NAME logic COMMAND logic_test)

add_executable(smoke_test
    tests/smoke_test.c
    ${CORE_SOURCES}
    ${RENDER_SOURCES}
    ${UI_SOURCES}
)
target_include_directories(smoke_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(smoke_test PRIVATE PkgConfig::SDL2 m)
target_compile_options(smoke_test PRIVATE -Wall -Wextra)
add_test(NAME smoke COMMAND smoke_test)
set_tests_properties(smoke PROPERTIES
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_RENDER_DRIVER=software")
