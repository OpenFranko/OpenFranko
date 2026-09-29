set(PRESENTS_STATE_TEST_TARGET presentsStateTest)
add_executable(${PRESENTS_STATE_TEST_TARGET}
    presentsStateTest.cpp)
target_link_libraries(${PRESENTS_STATE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Assets Audio Graphics Input States
            StatesPresents)
add_test(NAME ${PRESENTS_STATE_TEST_TARGET}
    COMMAND ${PRESENTS_STATE_TEST_TARGET})
