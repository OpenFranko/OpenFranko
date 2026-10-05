set(CONTROLLER_SYSTEM_TEST_TARGET controllerSystemTest)
add_executable(${CONTROLLER_SYSTEM_TEST_TARGET}
    controllerSystemTest.cpp)
target_link_libraries(${CONTROLLER_SYSTEM_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Input)
add_test(NAME ${CONTROLLER_SYSTEM_TEST_TARGET}
    COMMAND ${CONTROLLER_SYSTEM_TEST_TARGET})

set(PLATFORM_TEST_TARGET platformTest)
add_executable(${PLATFORM_TEST_TARGET}
    platformTest.cpp)
target_link_libraries(${PLATFORM_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Platform ${SDL2_TARGET})
add_test(NAME ${PLATFORM_TEST_TARGET} COMMAND ${PLATFORM_TEST_TARGET})
