set(AMAL_MOTION_TEST_TARGET amalMotionTest)
add_executable(${AMAL_MOTION_TEST_TARGET}
    amalMotionTest.cpp
)
target_link_libraries(${AMAL_MOTION_TEST_TARGET} PRIVATE Catch2::Catch2WithMain EffectsAnimation)
add_test(NAME ${AMAL_MOTION_TEST_TARGET} COMMAND ${AMAL_MOTION_TEST_TARGET})

set(CREDIT_SCROLL_TEST_TARGET creditScrollTest)
add_executable(${CREDIT_SCROLL_TEST_TARGET}
    creditScrollTest.cpp
)
target_link_libraries(${CREDIT_SCROLL_TEST_TARGET} PRIVATE Catch2::Catch2WithMain EffectsAnimation)
add_test(NAME ${CREDIT_SCROLL_TEST_TARGET} COMMAND ${CREDIT_SCROLL_TEST_TARGET})

set(AMAL_ANIM_TEST_TARGET amalAnimTest)
add_executable(${AMAL_ANIM_TEST_TARGET}
    amalAnimTest.cpp
)
target_link_libraries(${AMAL_ANIM_TEST_TARGET} PRIVATE Catch2::Catch2WithMain EffectsAnimation)
add_test(NAME ${AMAL_ANIM_TEST_TARGET} COMMAND ${AMAL_ANIM_TEST_TARGET})
