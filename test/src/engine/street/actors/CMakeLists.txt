set(ACTORS_TEST_TARGET actorsTest)
add_executable(${ACTORS_TEST_TARGET}
    actorsTest.cpp
)
target_link_libraries(${ACTORS_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Amal StreetActors)
add_test(NAME ${ACTORS_TEST_TARGET} COMMAND ${ACTORS_TEST_TARGET})
