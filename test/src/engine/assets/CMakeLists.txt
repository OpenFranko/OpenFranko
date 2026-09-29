set(ASSETS_TEST_TARGET assetsTest)
add_executable(${ASSETS_TEST_TARGET}
    assetsTest.cpp
)
target_link_libraries(${ASSETS_TEST_TARGET} PRIVATE Catch2::Catch2WithMain Assets)
add_test(NAME ${ASSETS_TEST_TARGET} COMMAND ${ASSETS_TEST_TARGET})
