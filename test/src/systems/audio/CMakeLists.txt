set(MIXER_TEST_TARGET mixerTest)
add_executable(${MIXER_TEST_TARGET}
    mixerTest.cpp)
target_link_libraries(${MIXER_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Audio)
add_test(NAME ${MIXER_TEST_TARGET} COMMAND ${MIXER_TEST_TARGET})

set(WAVE_TEST_TARGET waveTest)
add_executable(${WAVE_TEST_TARGET}
    waveTest.cpp)
target_link_libraries(${WAVE_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Audio)
add_test(NAME ${WAVE_TEST_TARGET} COMMAND ${WAVE_TEST_TARGET})

set(TRACKER_TEST_TARGET trackerTest)
add_executable(${TRACKER_TEST_TARGET}
    trackerTest.cpp)
target_link_libraries(${TRACKER_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Audio ${XMP_TARGET})
add_test(NAME ${TRACKER_TEST_TARGET} COMMAND ${TRACKER_TEST_TARGET})

set(AUDIO_DEVICE_TEST_TARGET audioDeviceTest)
add_executable(${AUDIO_DEVICE_TEST_TARGET}
    audioDeviceTest.cpp)
target_link_libraries(${AUDIO_DEVICE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain AudioSystem ${SDL2_TARGET})
add_test(NAME ${AUDIO_DEVICE_TEST_TARGET} COMMAND ${AUDIO_DEVICE_TEST_TARGET})

set(AUDIO_SYSTEM_TEST_TARGET audioSystemTest)
add_executable(${AUDIO_SYSTEM_TEST_TARGET}
    audioSystemTest.cpp)
target_link_libraries(${AUDIO_SYSTEM_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain AudioSystem ${SDL2_TARGET})
add_test(NAME ${AUDIO_SYSTEM_TEST_TARGET} COMMAND ${AUDIO_SYSTEM_TEST_TARGET})
