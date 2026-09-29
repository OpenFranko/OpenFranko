set(HIGH_SCORE_STATE_TEST_TARGET highScoreStateTest)
add_executable(${HIGH_SCORE_STATE_TEST_TARGET}
    highScoreStateTest.cpp)
target_link_libraries(${HIGH_SCORE_STATE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Amal Graphics States StatesHighScore
            StreetCore StreetScenes)
add_test(NAME ${HIGH_SCORE_STATE_TEST_TARGET}
    COMMAND ${HIGH_SCORE_STATE_TEST_TARGET})
