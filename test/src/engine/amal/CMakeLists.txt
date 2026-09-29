set(MACHINE_TEST_TARGET machineTest)
add_executable(${MACHINE_TEST_TARGET}
    machineTest.cpp)
target_link_libraries(${MACHINE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Amal StreetActors)
add_test(NAME ${MACHINE_TEST_TARGET} COMMAND ${MACHINE_TEST_TARGET})
