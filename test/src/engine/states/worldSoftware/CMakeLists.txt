set(WORLD_SOFTWARE_STATE_TEST_TARGET worldSoftwareStateTest)
add_executable(${WORLD_SOFTWARE_STATE_TEST_TARGET}
    worldSoftwareStateTest.cpp)
target_link_libraries(${WORLD_SOFTWARE_STATE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Assets Audio Common Graphics States
            StatesWorldSoftware)
add_test(NAME ${WORLD_SOFTWARE_STATE_TEST_TARGET}
    COMMAND ${WORLD_SOFTWARE_STATE_TEST_TARGET})
