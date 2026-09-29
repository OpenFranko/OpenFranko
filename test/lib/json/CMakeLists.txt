set(JSON_TEST_TARGET jsonTest)
add_executable(${JSON_TEST_TARGET}
    jsonTest.cpp)
target_link_libraries(${JSON_TEST_TARGET} PRIVATE Catch2::Catch2WithMain json)
add_test(NAME ${JSON_TEST_TARGET} COMMAND ${JSON_TEST_TARGET})
