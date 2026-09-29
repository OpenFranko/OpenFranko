set(TITLE_AND_STORY_STATE_TEST_TARGET titleAndStoryStateTest)
add_executable(${TITLE_AND_STORY_STATE_TEST_TARGET}
    titleAndStoryStateTest.cpp)
target_link_libraries(${TITLE_AND_STORY_STATE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Assets Audio Common Graphics Input States
            StatesTitleAndStory)
add_test(NAME ${TITLE_AND_STORY_STATE_TEST_TARGET}
    COMMAND ${TITLE_AND_STORY_STATE_TEST_TARGET})
