set(PALETTE_FADER_TEST_TARGET paletteFaderTest)
add_executable(${PALETTE_FADER_TEST_TARGET}
    paletteFaderTest.cpp)
target_link_libraries(${PALETTE_FADER_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain EffectsColor)
add_test(NAME ${PALETTE_FADER_TEST_TARGET} COMMAND ${PALETTE_FADER_TEST_TARGET})

set(PALETTE_FLASHER_TEST_TARGET paletteFlasherTest)
add_executable(${PALETTE_FLASHER_TEST_TARGET}
    paletteFlasherTest.cpp)
target_link_libraries(${PALETTE_FLASHER_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain EffectsColor)
add_test(NAME ${PALETTE_FLASHER_TEST_TARGET}
    COMMAND ${PALETTE_FLASHER_TEST_TARGET})

set(RAINBOW_TEST_TARGET rainbowTest)
add_executable(${RAINBOW_TEST_TARGET}
    rainbowTest.cpp)
target_link_libraries(${RAINBOW_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain EffectsColor)
add_test(NAME ${RAINBOW_TEST_TARGET} COMMAND ${RAINBOW_TEST_TARGET})
