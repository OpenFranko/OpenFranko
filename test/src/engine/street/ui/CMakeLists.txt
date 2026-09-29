set(LOADING_QUEUE_TEST_TARGET loadingQueueTest)
add_executable(${LOADING_QUEUE_TEST_TARGET}
    loadingQueueTest.cpp)
target_link_libraries(${LOADING_QUEUE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain StreetUi)
add_test(NAME ${LOADING_QUEUE_TEST_TARGET} COMMAND ${LOADING_QUEUE_TEST_TARGET})

set(STAGE_FRAME_TEST_TARGET stageFrameTest)
add_executable(${STAGE_FRAME_TEST_TARGET}
    stageFrameTest.cpp)
target_link_libraries(${STAGE_FRAME_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain StreetUi)
add_test(NAME ${STAGE_FRAME_TEST_TARGET} COMMAND ${STAGE_FRAME_TEST_TARGET})

set(STATUS_PANEL_TEST_TARGET statusPanelTest)
add_executable(${STATUS_PANEL_TEST_TARGET}
    statusPanelTest.cpp)
target_link_libraries(${STATUS_PANEL_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain StreetUi)
add_test(NAME ${STATUS_PANEL_TEST_TARGET} COMMAND ${STATUS_PANEL_TEST_TARGET})
