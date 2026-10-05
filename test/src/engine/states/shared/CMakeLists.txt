set(MUSIC_FADE_OUT_TEST_TARGET musicFadeOutTest)
add_executable(${MUSIC_FADE_OUT_TEST_TARGET}
    musicFadeOutTest.cpp)
target_link_libraries(${MUSIC_FADE_OUT_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Audio StatesShared)
add_test(NAME ${MUSIC_FADE_OUT_TEST_TARGET}
    COMMAND ${MUSIC_FADE_OUT_TEST_TARGET})

set(STAGE_STATE_TEST_TARGET stageStateTest)
add_executable(${STAGE_STATE_TEST_TARGET}
    stageStateTest.cpp)
target_link_libraries(${STAGE_STATE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Graphics States StatesShared StreetCore
            StreetScenes)
add_test(NAME ${STAGE_STATE_TEST_TARGET} COMMAND ${STAGE_STATE_TEST_TARGET})

set(STREET_CONTROLS_TEST_TARGET streetControlsTest)
add_executable(${STREET_CONTROLS_TEST_TARGET}
    streetControlsTest.cpp)
target_link_libraries(${STREET_CONTROLS_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain StatesShared)
add_test(NAME ${STREET_CONTROLS_TEST_TARGET}
    COMMAND ${STREET_CONTROLS_TEST_TARGET})

set(ENGINE_STREET_HOST_TEST_TARGET engineStreetHostTest)
add_executable(${ENGINE_STREET_HOST_TEST_TARGET}
    engineStreetHostTest.cpp)
target_link_libraries(${ENGINE_STREET_HOST_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Assets StatesShared StreetCore
            packedArchive)
add_test(NAME ${ENGINE_STREET_HOST_TEST_TARGET}
    COMMAND ${ENGINE_STREET_HOST_TEST_TARGET})

set(STEP_LOADER_TEST_TARGET stepLoaderTest)
add_executable(${STEP_LOADER_TEST_TARGET}
    stepLoaderTest.cpp)
target_link_libraries(${STEP_LOADER_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Assets StatesShared packedArchive)
add_test(NAME ${STEP_LOADER_TEST_TARGET} COMMAND ${STEP_LOADER_TEST_TARGET})

set(INTRO_STRIP_TEST_TARGET introStripTest)
add_executable(${INTRO_STRIP_TEST_TARGET}
    introStripTest.cpp)
target_link_libraries(${INTRO_STRIP_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Assets EffectsSequences Graphics
            StatesShared)
add_test(NAME ${INTRO_STRIP_TEST_TARGET} COMMAND ${INTRO_STRIP_TEST_TARGET})
