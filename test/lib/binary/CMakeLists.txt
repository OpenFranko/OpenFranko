set(BINARY_TEST_TARGET binaryTest)
add_executable(${BINARY_TEST_TARGET}
    binaryTest.cpp
)
target_link_libraries(${BINARY_TEST_TARGET} PRIVATE Catch2::Catch2WithMain binary)
add_test(NAME ${BINARY_TEST_TARGET} COMMAND ${BINARY_TEST_TARGET})
