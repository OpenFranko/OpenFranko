set(ENDING_STATE_TEST_TARGET endingStateTest)
add_executable(${ENDING_STATE_TEST_TARGET}
    endingStateTest.cpp)
target_link_libraries(${ENDING_STATE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Graphics States StatesEnding StreetCore
            StreetScenes StreetUi)
add_test(NAME ${ENDING_STATE_TEST_TARGET} COMMAND ${ENDING_STATE_TEST_TARGET})
