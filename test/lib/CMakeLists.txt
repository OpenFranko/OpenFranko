find_package(Catch2 REQUIRED)

set(READ_FILE_TEST_TARGET readFileTest)
add_executable(${READ_FILE_TEST_TARGET}
    filesystem/readFileTest.cpp
)
target_link_libraries(${READ_FILE_TEST_TARGET} PRIVATE Catch2::Catch2WithMain readFile)
add_test(NAME ${READ_FILE_TEST_TARGET} COMMAND ${READ_FILE_TEST_TARGET})

set(BACKWARD_LZ77_TEST_TARGET backwardLZ77Test)
add_executable(${BACKWARD_LZ77_TEST_TARGET}
    decompressor/backwardLZ77Test.cpp
)
target_link_libraries(${BACKWARD_LZ77_TEST_TARGET} PRIVATE Catch2::Catch2WithMain backwardLZ77)
add_test(NAME ${BACKWARD_LZ77_TEST_TARGET} COMMAND ${BACKWARD_LZ77_TEST_TARGET})

