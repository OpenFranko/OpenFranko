set(COMPILED_ACTORS_TEST_TARGET compiledActorsTest)
add_executable(${COMPILED_ACTORS_TEST_TARGET}
    compiledActorsTest.cpp)
target_link_libraries(${COMPILED_ACTORS_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Amal StreetActors StreetActorsCompiled)
add_test(NAME ${COMPILED_ACTORS_TEST_TARGET}
    COMMAND ${COMPILED_ACTORS_TEST_TARGET})
