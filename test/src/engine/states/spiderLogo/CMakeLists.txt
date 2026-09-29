set(SPIDER_LOGO_STATE_TEST_TARGET spiderLogoStateTest)
add_executable(${SPIDER_LOGO_STATE_TEST_TARGET}
    spiderLogoStateTest.cpp)
target_link_libraries(${SPIDER_LOGO_STATE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Assets Audio Graphics States
            StatesSpiderLogo)
add_test(NAME ${SPIDER_LOGO_STATE_TEST_TARGET}
    COMMAND ${SPIDER_LOGO_STATE_TEST_TARGET})
