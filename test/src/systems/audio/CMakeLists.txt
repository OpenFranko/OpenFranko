set(WAVE_TEST_TARGET waveTest)
add_executable(${WAVE_TEST_TARGET}
    waveTest.cpp
)
target_link_libraries(${WAVE_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Audio)
add_test(NAME ${WAVE_TEST_TARGET} COMMAND ${WAVE_TEST_TARGET})

set(MIXER_TEST_TARGET mixerTest)
add_executable(${MIXER_TEST_TARGET}
    mixerTest.cpp
)
target_link_libraries(${MIXER_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Audio)
add_test(NAME ${MIXER_TEST_TARGET} COMMAND ${MIXER_TEST_TARGET})
