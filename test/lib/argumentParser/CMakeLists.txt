set(ARGUMENT_PARSER_TEST_TARGET argumentParserTest)
add_executable(${ARGUMENT_PARSER_TEST_TARGET}
    argumentParserTest.cpp)
target_link_libraries(${ARGUMENT_PARSER_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain argumentParser)
add_test(NAME ${ARGUMENT_PARSER_TEST_TARGET}
    COMMAND ${ARGUMENT_PARSER_TEST_TARGET})
