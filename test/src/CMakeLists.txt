find_package(Catch2 REQUIRED)

set(PALETTE_FADER_TEST_TARGET paletteFaderTest)
add_executable(${PALETTE_FADER_TEST_TARGET}
    engine/effects/paletteFaderTest.cpp
)
target_link_libraries(${PALETTE_FADER_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Effects)
add_test(NAME ${PALETTE_FADER_TEST_TARGET} COMMAND ${PALETTE_FADER_TEST_TARGET})

set(PALETTE_FLASHER_TEST_TARGET paletteFlasherTest)
add_executable(${PALETTE_FLASHER_TEST_TARGET}
    engine/effects/paletteFlasherTest.cpp
)
target_link_libraries(${PALETTE_FLASHER_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Effects)
add_test(NAME ${PALETTE_FLASHER_TEST_TARGET} COMMAND ${PALETTE_FLASHER_TEST_TARGET})

set(FOTO_SEQUENCE_TEST_TARGET fotoSequenceTest)
add_executable(${FOTO_SEQUENCE_TEST_TARGET}
    engine/effects/fotoSequenceTest.cpp
)
target_link_libraries(${FOTO_SEQUENCE_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Effects)
add_test(NAME ${FOTO_SEQUENCE_TEST_TARGET} COMMAND ${FOTO_SEQUENCE_TEST_TARGET})

set(STORY_SEQUENCE_TEST_TARGET storySequenceTest)
add_executable(${STORY_SEQUENCE_TEST_TARGET}
    engine/effects/storySequenceTest.cpp
)
target_link_libraries(${STORY_SEQUENCE_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Effects)
add_test(NAME ${STORY_SEQUENCE_TEST_TARGET} COMMAND ${STORY_SEQUENCE_TEST_TARGET})

set(CODE_CARD_CHECK_TEST_TARGET codeCardCheckTest)
add_executable(${CODE_CARD_CHECK_TEST_TARGET}
    engine/effects/codeCardCheckTest.cpp
)
target_link_libraries(${CODE_CARD_CHECK_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Effects)
add_test(NAME ${CODE_CARD_CHECK_TEST_TARGET} COMMAND ${CODE_CARD_CHECK_TEST_TARGET})

set(AMAL_MOTION_TEST_TARGET amalMotionTest)
add_executable(${AMAL_MOTION_TEST_TARGET}
    engine/effects/amalMotionTest.cpp
)
target_link_libraries(${AMAL_MOTION_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Effects)
add_test(NAME ${AMAL_MOTION_TEST_TARGET} COMMAND ${AMAL_MOTION_TEST_TARGET})

set(CREDIT_SCROLL_TEST_TARGET creditScrollTest)
add_executable(${CREDIT_SCROLL_TEST_TARGET}
    engine/effects/creditScrollTest.cpp
)
target_link_libraries(${CREDIT_SCROLL_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Effects)
add_test(NAME ${CREDIT_SCROLL_TEST_TARGET} COMMAND ${CREDIT_SCROLL_TEST_TARGET})

set(MENU_SEQUENCE_TEST_TARGET menuSequenceTest)
add_executable(${MENU_SEQUENCE_TEST_TARGET}
    engine/effects/menuSequenceTest.cpp
)
target_link_libraries(${MENU_SEQUENCE_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Effects)
add_test(NAME ${MENU_SEQUENCE_TEST_TARGET} COMMAND ${MENU_SEQUENCE_TEST_TARGET})

set(INKEY_BUFFER_TEST_TARGET inkeyBufferTest)
add_executable(${INKEY_BUFFER_TEST_TARGET}
    engine/effects/inkeyBufferTest.cpp
)
target_link_libraries(${INKEY_BUFFER_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Effects)
add_test(NAME ${INKEY_BUFFER_TEST_TARGET} COMMAND ${INKEY_BUFFER_TEST_TARGET})

set(ATTRACT_SEQUENCE_TEST_TARGET attractSequenceTest)
add_executable(${ATTRACT_SEQUENCE_TEST_TARGET}
    engine/effects/attractSequenceTest.cpp
)
target_link_libraries(${ATTRACT_SEQUENCE_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Effects)
add_test(NAME ${ATTRACT_SEQUENCE_TEST_TARGET} COMMAND ${ATTRACT_SEQUENCE_TEST_TARGET})

set(AMAL_ANIM_TEST_TARGET amalAnimTest)
add_executable(${AMAL_ANIM_TEST_TARGET}
    engine/effects/amalAnimTest.cpp
)
target_link_libraries(${AMAL_ANIM_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Effects)
add_test(NAME ${AMAL_ANIM_TEST_TARGET} COMMAND ${AMAL_ANIM_TEST_TARGET})

set(CHARACTER_SELECTION_TEST_TARGET characterSelectionTest)
add_executable(${CHARACTER_SELECTION_TEST_TARGET}
    engine/effects/characterSelectionTest.cpp
)
target_link_libraries(${CHARACTER_SELECTION_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Effects)
add_test(NAME ${CHARACTER_SELECTION_TEST_TARGET} COMMAND ${CHARACTER_SELECTION_TEST_TARGET})

set(AMAL_MACHINE_TEST_TARGET amalMachineTest)
add_executable(${AMAL_MACHINE_TEST_TARGET}
    engine/amal/amalMachineTest.cpp
)
target_link_libraries(${AMAL_MACHINE_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Amal)
add_test(NAME ${AMAL_MACHINE_TEST_TARGET} COMMAND ${AMAL_MACHINE_TEST_TARGET})

set(AMAL_ACTORS_TEST_TARGET amalActorsTest)
add_executable(${AMAL_ACTORS_TEST_TARGET}
    engine/amal/amalActorsTest.cpp
)
target_link_libraries(${AMAL_ACTORS_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Amal)
add_test(NAME ${AMAL_ACTORS_TEST_TARGET} COMMAND ${AMAL_ACTORS_TEST_TARGET})

set(INDEXED_SURFACE_TEST_TARGET indexedSurfaceTest)
add_executable(${INDEXED_SURFACE_TEST_TARGET}
    engine/street/indexedSurfaceTest.cpp
)
target_link_libraries(${INDEXED_SURFACE_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Street)
add_test(NAME ${INDEXED_SURFACE_TEST_TARGET} COMMAND ${INDEXED_SURFACE_TEST_TARGET})

set(BOBS_TEST_TARGET bobsTest)
add_executable(${BOBS_TEST_TARGET}
    engine/street/bobsTest.cpp
)
target_link_libraries(${BOBS_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Street)
add_test(NAME ${BOBS_TEST_TARGET} COMMAND ${BOBS_TEST_TARGET})

set(STATUS_PANEL_TEST_TARGET statusPanelTest)
add_executable(${STATUS_PANEL_TEST_TARGET}
    engine/street/statusPanelTest.cpp
)
target_link_libraries(${STATUS_PANEL_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Street)
add_test(NAME ${STATUS_PANEL_TEST_TARGET} COMMAND ${STATUS_PANEL_TEST_TARGET})

set(STREET_LEVEL_SCRIPT_TEST_TARGET streetLevelScriptTest)
add_executable(${STREET_LEVEL_SCRIPT_TEST_TARGET}
    engine/street/streetLevelScriptTest.cpp
)
target_link_libraries(${STREET_LEVEL_SCRIPT_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Street)
add_test(NAME ${STREET_LEVEL_SCRIPT_TEST_TARGET} COMMAND ${STREET_LEVEL_SCRIPT_TEST_TARGET})

set(STREET_STAGE_TEST_TARGET streetStageTest)
add_executable(${STREET_STAGE_TEST_TARGET}
    engine/street/streetStageTest.cpp
)
target_link_libraries(${STREET_STAGE_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Street)
add_test(NAME ${STREET_STAGE_TEST_TARGET} COMMAND ${STREET_STAGE_TEST_TARGET})

set(BOSS_STAGE_TEST_TARGET bossStageTest)
add_executable(${BOSS_STAGE_TEST_TARGET}
    engine/street/bossStageTest.cpp
)
target_link_libraries(${BOSS_STAGE_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Street)
add_test(NAME ${BOSS_STAGE_TEST_TARGET} COMMAND ${BOSS_STAGE_TEST_TARGET})

set(LOADING_MOCK_TEST_TARGET loadingMockTest)
add_executable(${LOADING_MOCK_TEST_TARGET}
    engine/street/loadingMockTest.cpp
)
target_link_libraries(${LOADING_MOCK_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Street)
add_test(NAME ${LOADING_MOCK_TEST_TARGET} COMMAND ${LOADING_MOCK_TEST_TARGET})

set(RAINBOW_TEST_TARGET rainbowTest)
add_executable(${RAINBOW_TEST_TARGET}
    engine/effects/rainbowTest.cpp
)
target_link_libraries(${RAINBOW_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Effects)
add_test(NAME ${RAINBOW_TEST_TARGET} COMMAND ${RAINBOW_TEST_TARGET})

set(GAME_OVER_SCENE_TEST_TARGET gameOverSceneTest)
add_executable(${GAME_OVER_SCENE_TEST_TARGET}
    engine/street/gameOverSceneTest.cpp
)
target_link_libraries(${GAME_OVER_SCENE_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Street)
add_test(NAME ${GAME_OVER_SCENE_TEST_TARGET} COMMAND ${GAME_OVER_SCENE_TEST_TARGET})

set(HIGH_SCORE_TABLE_TEST_TARGET highScoreTableTest)
add_executable(${HIGH_SCORE_TABLE_TEST_TARGET}
    engine/street/highScoreTableTest.cpp
)
target_link_libraries(${HIGH_SCORE_TABLE_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Street)
add_test(NAME ${HIGH_SCORE_TABLE_TEST_TARGET} COMMAND ${HIGH_SCORE_TABLE_TEST_TARGET})

set(HIGH_SCORE_SCENE_TEST_TARGET highScoreSceneTest)
add_executable(${HIGH_SCORE_SCENE_TEST_TARGET}
    engine/street/highScoreSceneTest.cpp
)
target_link_libraries(${HIGH_SCORE_SCENE_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Street)
add_test(NAME ${HIGH_SCORE_SCENE_TEST_TARGET} COMMAND ${HIGH_SCORE_SCENE_TEST_TARGET})

set(CONTINUE_SCENE_TEST_TARGET continueSceneTest)
add_executable(${CONTINUE_SCENE_TEST_TARGET}
    engine/street/continueSceneTest.cpp
)
target_link_libraries(${CONTINUE_SCENE_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Street)
add_test(NAME ${CONTINUE_SCENE_TEST_TARGET} COMMAND ${CONTINUE_SCENE_TEST_TARGET})

set(CAR_STAGE_TEST_TARGET carStageTest)
add_executable(${CAR_STAGE_TEST_TARGET}
    engine/street/carStageTest.cpp
)
target_link_libraries(${CAR_STAGE_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Street)
add_test(NAME ${CAR_STAGE_TEST_TARGET} COMMAND ${CAR_STAGE_TEST_TARGET})

set(ENDING_SCENE_TEST_TARGET endingSceneTest)
add_executable(${ENDING_SCENE_TEST_TARGET}
    engine/street/endingSceneTest.cpp
)
target_link_libraries(${ENDING_SCENE_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Street)
add_test(NAME ${ENDING_SCENE_TEST_TARGET} COMMAND ${ENDING_SCENE_TEST_TARGET})

set(DOUBLE_BUFFER_TEST_TARGET doubleBufferTest)
add_executable(${DOUBLE_BUFFER_TEST_TARGET}
    engine/street/doubleBufferTest.cpp
)
target_link_libraries(${DOUBLE_BUFFER_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Street)
add_test(NAME ${DOUBLE_BUFFER_TEST_TARGET} COMMAND ${DOUBLE_BUFFER_TEST_TARGET})

set(CHEAT_CODES_TEST_TARGET cheatCodesTest)
add_executable(${CHEAT_CODES_TEST_TARGET}
    engine/street/cheatCodesTest.cpp
)
target_link_libraries(${CHEAT_CODES_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Street)
add_test(NAME ${CHEAT_CODES_TEST_TARGET} COMMAND ${CHEAT_CODES_TEST_TARGET})

set(STAGE_FRAME_TEST_TARGET stageFrameTest)
add_executable(${STAGE_FRAME_TEST_TARGET}
    engine/street/stageFrameTest.cpp
)
target_link_libraries(${STAGE_FRAME_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Street)
add_test(NAME ${STAGE_FRAME_TEST_TARGET} COMMAND ${STAGE_FRAME_TEST_TARGET})

set(AMIGA_DISPLAY_TEST_TARGET amigaDisplayTest)
add_executable(${AMIGA_DISPLAY_TEST_TARGET}
    engine/effects/amigaDisplayTest.cpp
)
target_link_libraries(${AMIGA_DISPLAY_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Effects)
add_test(NAME ${AMIGA_DISPLAY_TEST_TARGET} COMMAND ${AMIGA_DISPLAY_TEST_TARGET})

set(BITMAP_TEST_TARGET bitmapTest)
add_executable(${BITMAP_TEST_TARGET}
    systems/bitmapTest.cpp
)
target_link_libraries(${BITMAP_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Graphics)
add_test(NAME ${BITMAP_TEST_TARGET} COMMAND ${BITMAP_TEST_TARGET})

set(CANVAS_TEST_TARGET canvasTest)
add_executable(${CANVAS_TEST_TARGET}
    systems/canvasTest.cpp
)
target_link_libraries(${CANVAS_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Graphics)
add_test(NAME ${CANVAS_TEST_TARGET} COMMAND ${CANVAS_TEST_TARGET})

set(CONTROLLER_SYSTEM_TEST_TARGET controllerSystemTest)
add_executable(${CONTROLLER_SYSTEM_TEST_TARGET}
    systems/controllerSystemTest.cpp
)
target_link_libraries(${CONTROLLER_SYSTEM_TEST_TARGET} PRIVATE Catch2::Catch2WithMain ControllerSystem)
add_test(NAME ${CONTROLLER_SYSTEM_TEST_TARGET} COMMAND ${CONTROLLER_SYSTEM_TEST_TARGET})

set(WAVE_TEST_TARGET waveTest)
add_executable(${WAVE_TEST_TARGET}
    systems/waveTest.cpp
)
target_link_libraries(${WAVE_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Mixer)
add_test(NAME ${WAVE_TEST_TARGET} COMMAND ${WAVE_TEST_TARGET})

set(MIXER_TEST_TARGET mixerTest)
add_executable(${MIXER_TEST_TARGET}
    systems/mixerTest.cpp
)
target_link_libraries(${MIXER_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Mixer)
add_test(NAME ${MIXER_TEST_TARGET} COMMAND ${MIXER_TEST_TARGET})

set(DISPLAY_TEST_TARGET displayTest)
add_executable(${DISPLAY_TEST_TARGET}
    systems/displayTest.cpp
)
target_link_libraries(${DISPLAY_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Graphics)
add_test(NAME ${DISPLAY_TEST_TARGET} COMMAND ${DISPLAY_TEST_TARGET})
