cmake_minimum_required(VERSION 3.16)
project(shogun_online C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
if(MSVC)
    set(CMAKE_C_STANDARD 11)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

set(SHOGUN_COMPILE_OPTIONS -O2 -Wall -Wextra)
set(SHOGUN_TEST_COMPILE_OPTIONS -UNDEBUG -Wall -Wextra)
if(MSVC)
    set(SHOGUN_COMPILE_OPTIONS /W4)
    set(SHOGUN_TEST_COMPILE_OPTIONS /UNDEBUG /W4)
endif()

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(MSVC AND VCPKG_TARGET_TRIPLET MATCHES "-static")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)
endif()

find_package(SDL2 CONFIG QUIET)
if(SDL2_FOUND AND TARGET SDL2::SDL2)
    if(WIN32 AND TARGET SDL2::SDL2main)
        set(SHOGUN_SDL2 SDL2::SDL2main SDL2::SDL2)
    else()
        set(SHOGUN_SDL2 SDL2::SDL2)
    endif()
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
    set(SHOGUN_SDL2 PkgConfig::SDL2)
endif()

if(MSVC)
    set(SHOGUN_MATH_LIB)
else()
    set(SHOGUN_MATH_LIB m)
endif()

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
set(ONLINE_COMMON_SOURCES
    protocol.c
    socket_util.c
)
set(ONLINE_CLIENT_SOURCES
    ${ONLINE_COMMON_SOURCES}
    client.c
)

add_executable(shogun src/ui/main.c ${CORE_SOURCES} ${RENDER_SOURCES}
    ${UI_SOURCES} ${ONLINE_CLIENT_SOURCES})
target_include_directories(shogun PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(shogun PRIVATE ${SHOGUN_SDL2} ${SHOGUN_MATH_LIB})
if(WIN32)
    set_target_properties(shogun PROPERTIES WIN32_EXECUTABLE TRUE)
    if(MSVC)
        target_link_options(shogun PRIVATE /ENTRY:mainCRTStartup)
    endif()
    target_link_libraries(shogun PRIVATE ws2_32)
endif()
target_compile_options(shogun PRIVATE ${SHOGUN_COMPILE_OPTIONS})

add_executable(shogun-server server_main.c server.c
    ${ONLINE_COMMON_SOURCES})
target_include_directories(shogun-server PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}")
if(WIN32)
    target_link_libraries(shogun-server PRIVATE ws2_32)
endif()
target_compile_options(shogun-server PRIVATE ${SHOGUN_COMPILE_OPTIONS})

enable_testing()

add_executable(logic_test
    tests/logic_test.c
    src/render/glyph.c
    src/render/layout.c
    ${CORE_SOURCES}
    src/ui/menu.c
)
target_include_directories(logic_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}")
if(SHOGUN_MATH_LIB)
    target_link_libraries(logic_test PRIVATE ${SHOGUN_MATH_LIB})
endif()
target_compile_options(logic_test PRIVATE ${SHOGUN_TEST_COMPILE_OPTIONS})
add_test(NAME logic COMMAND logic_test)

add_executable(smoke_test
    tests/smoke_test.c
    ${CORE_SOURCES}
    ${RENDER_SOURCES}
    ${UI_SOURCES}
)
target_include_directories(smoke_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(smoke_test PRIVATE ${SHOGUN_SDL2} ${SHOGUN_MATH_LIB})
target_compile_options(smoke_test PRIVATE ${SHOGUN_COMPILE_OPTIONS})
add_test(NAME smoke COMMAND smoke_test)
set_tests_properties(smoke PROPERTIES
    ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_RENDER_DRIVER=software")

add_executable(online_test
    tests/online_test.c
    ${CORE_SOURCES}
    src/render/layout.c
    ${ONLINE_CLIENT_SOURCES}
    server.c
)
target_include_directories(online_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}")
if(SHOGUN_MATH_LIB)
    target_link_libraries(online_test PRIVATE ${SHOGUN_MATH_LIB})
endif()
if(WIN32)
    target_link_libraries(online_test PRIVATE ws2_32)
endif()
target_compile_options(online_test PRIVATE ${SHOGUN_TEST_COMPILE_OPTIONS})
add_test(NAME online COMMAND online_test)
