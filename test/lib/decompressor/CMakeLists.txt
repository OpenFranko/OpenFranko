set(BACKWARD_LZ77_TEST_TARGET backwardLZ77Test)
add_executable(${BACKWARD_LZ77_TEST_TARGET}
    backwardLZ77Test.cpp
)
target_link_libraries(${BACKWARD_LZ77_TEST_TARGET} PRIVATE Catch2::Catch2WithMain backwardLZ77)
add_test(NAME ${BACKWARD_LZ77_TEST_TARGET} COMMAND ${BACKWARD_LZ77_TEST_TARGET})
