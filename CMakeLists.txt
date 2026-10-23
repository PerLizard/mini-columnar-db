cmake_minimum_required(VERSION 3.16)

project(MiniColumnarDB LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(mini_db
    src/main.cpp
)

target_include_directories(mini_db PRIVATE include)

target_compile_options(mini_db PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)