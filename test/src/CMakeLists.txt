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
