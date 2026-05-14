cmake_minimum_required(VERSION 3.10)

project(OpenFranko)

option(BUILD_TESTS "Build tests" OFF)
option(BUILD_TOOLS "Build tools" OFF)

include(FindPkgConfig)
include(GNUInstallDirs)

add_subdirectory(lib)

if (BUILD_TOOLS)
    add_subdirectory(tools)
endif()

add_executable(OpenFranko src/main.cpp)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

