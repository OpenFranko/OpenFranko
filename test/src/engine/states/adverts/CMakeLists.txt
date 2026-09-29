set(ADVERTS_STATE_TEST_TARGET advertsStateTest)
add_executable(${ADVERTS_STATE_TEST_TARGET}
    advertsStateTest.cpp)
target_link_libraries(${ADVERTS_STATE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Assets Common Graphics Input States
            StatesAdverts)
add_test(NAME ${ADVERTS_STATE_TEST_TARGET} COMMAND ${ADVERTS_STATE_TEST_TARGET})
