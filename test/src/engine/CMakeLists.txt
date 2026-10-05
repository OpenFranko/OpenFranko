add_subdirectory(amal)
add_subdirectory(assets)
add_subdirectory(effects)
add_subdirectory(states)
add_subdirectory(street)

set(AMIGA_DISPLAY_TEST_TARGET amigaDisplayTest)
add_executable(${AMIGA_DISPLAY_TEST_TARGET}
    amigaDisplayTest.cpp)
target_link_libraries(${AMIGA_DISPLAY_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Common)
add_test(NAME ${AMIGA_DISPLAY_TEST_TARGET} COMMAND ${AMIGA_DISPLAY_TEST_TARGET})

set(INKEY_BUFFER_TEST_TARGET inkeyBufferTest)
add_executable(${INKEY_BUFFER_TEST_TARGET}
    inkeyBufferTest.cpp)
target_link_libraries(${INKEY_BUFFER_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Common)
add_test(NAME ${INKEY_BUFFER_TEST_TARGET} COMMAND ${INKEY_BUFFER_TEST_TARGET})

set(MERSENNE_TWISTER_TEST_TARGET mersenneTwisterTest)
add_executable(${MERSENNE_TWISTER_TEST_TARGET}
    mersenneTwisterTest.cpp)
target_link_libraries(${MERSENNE_TWISTER_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Common)
add_test(NAME ${MERSENNE_TWISTER_TEST_TARGET}
    COMMAND ${MERSENNE_TWISTER_TEST_TARGET})

set(ENGINE_TEST_TARGET engineTest)
add_executable(${ENGINE_TEST_TARGET}
    engineTest.cpp)
target_link_libraries(${ENGINE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Engine ${SDL2_TARGET})
add_test(NAME ${ENGINE_TEST_TARGET} COMMAND ${ENGINE_TEST_TARGET})
