set(CONTINUE_STATE_TEST_TARGET continueStateTest)
add_executable(${CONTINUE_STATE_TEST_TARGET}
    continueStateTest.cpp)
target_link_libraries(${CONTINUE_STATE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Amal Graphics States StatesContinueSelect
            StreetCore StreetScenes)
add_test(NAME ${CONTINUE_STATE_TEST_TARGET}
    COMMAND ${CONTINUE_STATE_TEST_TARGET})
