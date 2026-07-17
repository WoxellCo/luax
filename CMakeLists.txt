cmake_minimum_required(VERSION 3.28)

project(luax)

set(CMAKE_C_STANDARD           99)
set(CMAKE_C_STANDARD_REQUIRED YES)
set(CMAKE_C_EXTENSIONS        OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(${PROJECT_NAME} STATIC src/luax.c
    src/luax_regex.c
    src/luax_memory.c
    src/nx/list.c src/nx/vector.c src/nx/stack.c
    src/main.c)

add_executable(${PROJECT_NAME}c src/luax.c
    src/luax_regex.c
    src/luax_memory.c
    src/nx/list.c src/nx/vector.c src/nx/stack.c
    src/main.c)

include_directories(include)

add_subdirectory(pcre2)
target_link_libraries(${PROJECT_NAME} pcre2-8)
target_link_libraries(${PROJECT_NAME}c pcre2-8)