set(KNEE_ANIMATION_STATE_TEST_TARGET kneeAnimationStateTest)
add_executable(${KNEE_ANIMATION_STATE_TEST_TARGET}
    kneeAnimationStateTest.cpp)
target_link_libraries(${KNEE_ANIMATION_STATE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Assets Audio Graphics Input States
            StatesKneeAnimation)
add_test(NAME ${KNEE_ANIMATION_STATE_TEST_TARGET}
    COMMAND ${KNEE_ANIMATION_STATE_TEST_TARGET})
