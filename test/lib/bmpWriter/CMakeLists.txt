set(BMP_WRITER_TEST_TARGET bmpWriterTest)
add_executable(${BMP_WRITER_TEST_TARGET}
    bmpWriterTest.cpp)
target_link_libraries(${BMP_WRITER_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain binary bmpWriter)
add_test(NAME ${BMP_WRITER_TEST_TARGET} COMMAND ${BMP_WRITER_TEST_TARGET})
