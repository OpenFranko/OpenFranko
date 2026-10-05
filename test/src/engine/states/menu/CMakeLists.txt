set(MENU_STATE_TEST_TARGET menuStateTest)
add_executable(${MENU_STATE_TEST_TARGET}
    menuStateTest.cpp)
target_link_libraries(${MENU_STATE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Amal Assets Audio Common EffectsSequences
            Graphics Input States StatesMenu)
add_test(NAME ${MENU_STATE_TEST_TARGET} COMMAND ${MENU_STATE_TEST_TARGET})

set(MENU_PREFETCH_TEST_TARGET menuPrefetchTest)
add_executable(${MENU_PREFETCH_TEST_TARGET}
    menuPrefetchTest.cpp)
target_link_libraries(${MENU_PREFETCH_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Assets Audio AudioSystem Common
            EffectsSequences Graphics Input States StatesMenu)
add_test(NAME ${MENU_PREFETCH_TEST_TARGET} COMMAND ${MENU_PREFETCH_TEST_TARGET})
set_tests_properties(${MENU_PREFETCH_TEST_TARGET}
    PROPERTIES ENVIRONMENT "SDL_AUDIODRIVER=dummy")
