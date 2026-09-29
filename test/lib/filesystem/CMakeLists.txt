set(READ_FILE_TEST_TARGET readFileTest)
add_executable(${READ_FILE_TEST_TARGET}
    readFileTest.cpp)
target_link_libraries(${READ_FILE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain readFile)
add_test(NAME ${READ_FILE_TEST_TARGET} COMMAND ${READ_FILE_TEST_TARGET})

set(WRITE_FILE_TEST_TARGET writeFileTest)
add_executable(${WRITE_FILE_TEST_TARGET}
    writeFileTest.cpp)
target_link_libraries(${WRITE_FILE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain readFile writeFile)
add_test(NAME ${WRITE_FILE_TEST_TARGET} COMMAND ${WRITE_FILE_TEST_TARGET})
