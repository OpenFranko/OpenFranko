set(CHEAT_CODES_TEST_TARGET cheatCodesTest)
add_executable(${CHEAT_CODES_TEST_TARGET}
    cheatCodesTest.cpp
)
target_link_libraries(${CHEAT_CODES_TEST_TARGET} PRIVATE Catch2::Catch2WithMain StreetSession)
add_test(NAME ${CHEAT_CODES_TEST_TARGET} COMMAND ${CHEAT_CODES_TEST_TARGET})
