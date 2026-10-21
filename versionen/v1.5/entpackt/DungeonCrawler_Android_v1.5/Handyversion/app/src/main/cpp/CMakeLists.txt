cmake_minimum_required(VERSION 3.22.1)
project(dungeon_crawler_android LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(PLATFORM "Android" CACHE STRING "raylib target platform" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(WITH_PIC ON CACHE BOOL "" FORCE)

set(RAYLIB_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../third_party/raylib")
add_subdirectory("${RAYLIB_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/raylib")

file(GLOB GAME_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/game/*.c"
)

add_library(dungeon_crawler SHARED
    ${GAME_SOURCES}
    "${CMAKE_CURRENT_LIST_DIR}/mobile_input.c"
)

target_include_directories(dungeon_crawler PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/game"
    "${CMAKE_CURRENT_LIST_DIR}"
)

target_compile_definitions(dungeon_crawler PRIVATE PLATFORM_ANDROID)
target_compile_options(dungeon_crawler PRIVATE
    -Wall
    -Wextra
    -Wno-unused-parameter
)

target_link_libraries(dungeon_crawler PRIVATE raylib android log EGL GLESv2 OpenSLES atomic m)
target_link_options(dungeon_crawler PRIVATE
    "-Wl,-u,ANativeActivity_onCreate"
    "-Wl,--wrap=fopen"
)
