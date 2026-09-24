set(PROJECT_NAME OpenFranko)

cmake_minimum_required(VERSION 3.10)

project(${PROJECT_NAME})

option(BUILD_TESTS "Build tests" OFF)
option(BUILD_TOOLS "Build tools" OFF)

include(FindPkgConfig)
include(GNUInstallDirs)

find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_mixer REQUIRED)
pkg_check_modules(XMP REQUIRED IMPORTED_TARGET libxmp)
include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS} ${SDL2_MIXER_INCLUDE_DIRS} ${XMP_INCLUDE_DIRS})

add_subdirectory(src)

if (BUILD_TOOLS)
    add_subdirectory(lib)
    add_subdirectory(tools)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

