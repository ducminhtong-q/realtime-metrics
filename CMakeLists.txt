cmake_minimum_required(VERSION 3.16)
project(realtime_metrics CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

include_directories(include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(metrics_core INTERFACE)
target_include_directories(metrics_core INTERFACE include)

add_executable(metrics_server src/main.cpp src/tcp_server.cpp)
target_link_libraries(metrics_server PRIVATE metrics_core pthread)

option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    enable_testing()

    add_executable(test_sliding_window tests/test_sliding_window.cpp)
    target_link_libraries(test_sliding_window PRIVATE metrics_core gtest_main)
    add_test(NAME test_sliding_window COMMAND test_sliding_window)
endif()