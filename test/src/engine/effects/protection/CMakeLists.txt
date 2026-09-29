set(CODE_CARD_CHECK_TEST_TARGET codeCardCheckTest)
add_executable(${CODE_CARD_CHECK_TEST_TARGET}
    codeCardCheckTest.cpp
)
target_link_libraries(${CODE_CARD_CHECK_TEST_TARGET} PRIVATE Catch2::Catch2WithMain EffectsProtection)
add_test(NAME ${CODE_CARD_CHECK_TEST_TARGET} COMMAND ${CODE_CARD_CHECK_TEST_TARGET})
