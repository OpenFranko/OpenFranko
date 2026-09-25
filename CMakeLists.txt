set(PROJECT_NAME OpenFranko)

cmake_minimum_required(VERSION 3.10)

project(${PROJECT_NAME})

option(BUILD_TESTS "Build tests" OFF)
option(BUILD_TOOLS "Build tools" OFF)
option(SKIP_COPY_PROTECTION "Skip the code card checks" OFF)

include(FindPkgConfig)
include(GNUInstallDirs)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

if(MINGW)
    find_package(libxmp REQUIRED)
    set(SDL2_TARGET SDL2::SDL2-static)
    set(XMP_TARGET libxmp::xmp_static)
    add_link_options(-static)
else()
    pkg_check_modules(XMP REQUIRED IMPORTED_TARGET libxmp)
    include_directories(${XMP_INCLUDE_DIRS})
    set(SDL2_TARGET SDL2::SDL2)
    set(XMP_TARGET PkgConfig::XMP)
endif()

add_subdirectory(src)

if (BUILD_TOOLS OR BUILD_TESTS)
    add_subdirectory(lib)
endif()

if (BUILD_TOOLS)
    add_subdirectory(tools)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

