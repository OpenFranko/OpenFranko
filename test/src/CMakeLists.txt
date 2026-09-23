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