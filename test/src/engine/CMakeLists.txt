add_subdirectory(amal)
add_subdirectory(assets)
add_subdirectory(effects)
add_subdirectory(street)

set(INKEY_BUFFER_TEST_TARGET inkeyBufferTest)
add_executable(${INKEY_BUFFER_TEST_TARGET}
    inkeyBufferTest.cpp
)
target_link_libraries(${INKEY_BUFFER_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Common)
add_test(NAME ${INKEY_BUFFER_TEST_TARGET} COMMAND ${INKEY_BUFFER_TEST_TARGET})

set(AMIGA_DISPLAY_TEST_TARGET amigaDisplayTest)
add_executable(${AMIGA_DISPLAY_TEST_TARGET}
    amigaDisplayTest.cpp
)
target_link_libraries(${AMIGA_DISPLAY_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Common)
add_test(NAME ${AMIGA_DISPLAY_TEST_TARGET} COMMAND ${AMIGA_DISPLAY_TEST_TARGET})
