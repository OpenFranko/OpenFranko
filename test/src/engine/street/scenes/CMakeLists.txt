set(BOSS_STAGE_TEST_TARGET bossStageTest)
add_executable(${BOSS_STAGE_TEST_TARGET}
    bossStageTest.cpp)
target_link_libraries(${BOSS_STAGE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Input StreetCore StreetScenes StreetUi)
add_test(NAME ${BOSS_STAGE_TEST_TARGET} COMMAND ${BOSS_STAGE_TEST_TARGET})

set(CAR_STAGE_TEST_TARGET carStageTest)
add_executable(${CAR_STAGE_TEST_TARGET}
    carStageTest.cpp)
target_link_libraries(${CAR_STAGE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Common Input StreetCore StreetScenes)
add_test(NAME ${CAR_STAGE_TEST_TARGET} COMMAND ${CAR_STAGE_TEST_TARGET})

set(CONTINUE_SCENE_TEST_TARGET continueSceneTest)
add_executable(${CONTINUE_SCENE_TEST_TARGET}
    continueSceneTest.cpp)
target_link_libraries(${CONTINUE_SCENE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Common Input StreetCore StreetScenes
            StreetUi)
add_test(NAME ${CONTINUE_SCENE_TEST_TARGET}
    COMMAND ${CONTINUE_SCENE_TEST_TARGET})

set(ENDING_SCENE_TEST_TARGET endingSceneTest)
add_executable(${ENDING_SCENE_TEST_TARGET}
    endingSceneTest.cpp)
target_link_libraries(${ENDING_SCENE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Common Input StreetCore StreetScenes
            StreetUi)
add_test(NAME ${ENDING_SCENE_TEST_TARGET} COMMAND ${ENDING_SCENE_TEST_TARGET})

set(GAME_OVER_SCENE_TEST_TARGET gameOverSceneTest)
add_executable(${GAME_OVER_SCENE_TEST_TARGET}
    gameOverSceneTest.cpp)
target_link_libraries(${GAME_OVER_SCENE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain EffectsColor Input StreetCore StreetScenes
            StreetUi)
add_test(NAME ${GAME_OVER_SCENE_TEST_TARGET}
    COMMAND ${GAME_OVER_SCENE_TEST_TARGET})

set(HIGH_SCORE_SCENE_TEST_TARGET highScoreSceneTest)
add_executable(${HIGH_SCORE_SCENE_TEST_TARGET}
    highScoreSceneTest.cpp)
target_link_libraries(${HIGH_SCORE_SCENE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain StreetCore StreetScenes StreetUi)
add_test(NAME ${HIGH_SCORE_SCENE_TEST_TARGET}
    COMMAND ${HIGH_SCORE_SCENE_TEST_TARGET})

set(STREET_STAGE_TEST_TARGET streetStageTest)
add_executable(${STREET_STAGE_TEST_TARGET}
    streetStageTest.cpp)
target_link_libraries(${STREET_STAGE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Input StreetCore StreetScenes)
add_test(NAME ${STREET_STAGE_TEST_TARGET} COMMAND ${STREET_STAGE_TEST_TARGET})

set(ENDING_SCENE_FLOW_TEST_TARGET endingSceneFlowTest)
add_executable(${ENDING_SCENE_FLOW_TEST_TARGET}
    endingSceneFlowTest.cpp)
target_link_libraries(${ENDING_SCENE_FLOW_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Common Input StreetCore StreetScenes
            StreetUi)
add_test(NAME ${ENDING_SCENE_FLOW_TEST_TARGET}
    COMMAND ${ENDING_SCENE_FLOW_TEST_TARGET})

set(GAME_OVER_SCENE_FLOW_TEST_TARGET gameOverSceneFlowTest)
add_executable(${GAME_OVER_SCENE_FLOW_TEST_TARGET}
    gameOverSceneFlowTest.cpp)
target_link_libraries(${GAME_OVER_SCENE_FLOW_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Input StreetCore StreetScenes StreetUi)
add_test(NAME ${GAME_OVER_SCENE_FLOW_TEST_TARGET}
    COMMAND ${GAME_OVER_SCENE_FLOW_TEST_TARGET})
