
cmake_minimum_required(VERSION 3.16)

project(MiniColumnarDB LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_SANITIZERS "Enable ASan and UBSan" OFF)

add_library(mini_db_lib INTERFACE)
target_include_directories(mini_db_lib INTERFACE
    ${PROJECT_SOURCE_DIR}/include
)

add_executable(mini_db src/main.cpp)
target_link_libraries(mini_db PRIVATE mini_db_lib)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mini_db_lib INTERFACE
        -Wall -Wextra -Wpedantic
    )

    if(ENABLE_SANITIZERS)
        target_compile_options(mini_db_lib INTERFACE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
        )
        target_link_options(mini_db_lib INTERFACE
            -fsanitize=address,undefined
        )
    endif()
endif()

include(CTest)

if(BUILD_TESTING)
    include(FetchContent)

    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.16.0.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    FetchContent_MakeAvailable(googletest)

    add_executable(mini_db_tests
        tests/aggregate_test.cpp
        tests/column_test.cpp
        tests/filter_test.cpp
        tests/projection_test.cpp
        tests/table_test.cpp
    )

    target_link_libraries(mini_db_tests PRIVATE
        mini_db_lib
        GTest::gtest_main
    )

    include(GoogleTest)
    gtest_discover_tests(mini_db_tests)
endif()
