find_package(Catch2 REQUIRED)

set(BACKWARD_LZ77_TEST_TARGET backwardLZ77Test)
add_executable(${BACKWARD_LZ77_TEST_TARGET}
    decompressor/backwardLZ77Test.cpp
)
target_link_libraries(${BACKWARD_LZ77_TEST_TARGET} PRIVATE Catch2::Catch2WithMain)
add_test(NAME ${BACKWARD_LZ77_TEST_TARGET} COMMAND ${BACKWARD_LZ77_TEST_TARGET})

