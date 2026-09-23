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