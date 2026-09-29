set(GAME_OVER_STATE_TEST_TARGET gameOverStateTest)
add_executable(${GAME_OVER_STATE_TEST_TARGET}
    gameOverStateTest.cpp)
target_link_libraries(${GAME_OVER_STATE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Graphics States StatesGameOver StreetCore
            StreetScenes)
add_test(NAME ${GAME_OVER_STATE_TEST_TARGET}
    COMMAND ${GAME_OVER_STATE_TEST_TARGET})
