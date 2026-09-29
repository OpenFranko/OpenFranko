set(MIRAGE_STATE_TEST_TARGET mirageStateTest)
add_executable(${MIRAGE_STATE_TEST_TARGET}
    mirageStateTest.cpp)
target_link_libraries(${MIRAGE_STATE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Assets Common Graphics States StatesMirage)
add_test(NAME ${MIRAGE_STATE_TEST_TARGET} COMMAND ${MIRAGE_STATE_TEST_TARGET})
