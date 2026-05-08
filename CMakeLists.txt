cmake_minimum_required(VERSION 3.10)

project(OpenFranko)

option(BUILD_TESTS "Build tests" OFF)

add_subdirectory(lib)
add_subdirectory(tools)

add_executable(OpenFranko src/main.cpp)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

