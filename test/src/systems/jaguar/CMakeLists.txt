set(FRAME_BUILDER_TEST_TARGET frameBuilderTest)
add_executable(${FRAME_BUILDER_TEST_TARGET}
    frameBuilderTest.cpp
    ${PROJECT_SOURCE_DIR}/src/systems/jaguar/FrameBuilder.cpp
    ${PROJECT_SOURCE_DIR}/src/systems/jaguar/ObjectList.cpp)
target_link_libraries(${FRAME_BUILDER_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Graphics)
add_test(NAME ${FRAME_BUILDER_TEST_TARGET} COMMAND ${FRAME_BUILDER_TEST_TARGET})
