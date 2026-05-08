find_package(Catch2 REQUIRED)

file(GLOB_RECURSE SRC *cpp)

set(TEST_TARGET ${PROJECT_NAME}Tests)

add_library(testmain STATIC main.cpp)
target_link_libraries(testmain Catch2::Catch2)

set(BACKWARD_LZ77_TEST_TARGET backwardLZ77Test)
add_executable(${BACKWARD_LZ77_TEST_TARGET} decompressor/backwardLZ77Test.cpp)
target_link_libraries(${BACKWARD_LZ77_TEST_TARGET} testmain)
add_test(NAME ${BACKWARD_LZ77_TEST_TARGET} COMMAND ${BACKWARD_LZ77_TEST_TARGET})

