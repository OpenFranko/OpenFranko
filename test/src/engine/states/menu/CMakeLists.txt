set(MENU_STATE_TEST_TARGET menuStateTest)
add_executable(${MENU_STATE_TEST_TARGET}
    menuStateTest.cpp)
target_link_libraries(${MENU_STATE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Amal Assets Audio Common EffectsSequences
            Graphics Input States StatesMenu)
add_test(NAME ${MENU_STATE_TEST_TARGET} COMMAND ${MENU_STATE_TEST_TARGET})
