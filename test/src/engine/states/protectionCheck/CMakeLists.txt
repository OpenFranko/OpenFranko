set(PROTECTION_CHECK_STATE_TEST_TARGET protectionCheckStateTest)
add_executable(${PROTECTION_CHECK_STATE_TEST_TARGET}
    protectionCheckStateTest.cpp)
target_link_libraries(${PROTECTION_CHECK_STATE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Assets Audio Common Graphics States
            StatesProtectionCheck StreetUi)
add_test(NAME ${PROTECTION_CHECK_STATE_TEST_TARGET}
    COMMAND ${PROTECTION_CHECK_STATE_TEST_TARGET})
