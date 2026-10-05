set(BITMAP_TEST_TARGET bitmapTest)
add_executable(${BITMAP_TEST_TARGET}
    bitmapTest.cpp)
target_link_libraries(${BITMAP_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Graphics)
add_test(NAME ${BITMAP_TEST_TARGET} COMMAND ${BITMAP_TEST_TARGET})

set(CANVAS_TEST_TARGET canvasTest)
add_executable(${CANVAS_TEST_TARGET}
    canvasTest.cpp)
target_link_libraries(${CANVAS_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Graphics)
add_test(NAME ${CANVAS_TEST_TARGET} COMMAND ${CANVAS_TEST_TARGET})

set(DISPLAY_TEST_TARGET displayTest)
add_executable(${DISPLAY_TEST_TARGET}
    displayTest.cpp)
target_link_libraries(${DISPLAY_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Graphics)
add_test(NAME ${DISPLAY_TEST_TARGET} COMMAND ${DISPLAY_TEST_TARGET})

set(INDEXED_RASTERIZER_TEST_TARGET indexedRasterizerTest)
add_executable(${INDEXED_RASTERIZER_TEST_TARGET}
    indexedRasterizerTest.cpp)
target_link_libraries(${INDEXED_RASTERIZER_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Graphics)
add_test(NAME ${INDEXED_RASTERIZER_TEST_TARGET}
    COMMAND ${INDEXED_RASTERIZER_TEST_TARGET})

set(SCREEN_MODE_TEST_TARGET screenModeTest)
add_executable(${SCREEN_MODE_TEST_TARGET}
    screenModeTest.cpp)
target_link_libraries(${SCREEN_MODE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Graphics)
add_test(NAME ${SCREEN_MODE_TEST_TARGET} COMMAND ${SCREEN_MODE_TEST_TARGET})

set(VIDEO_SYSTEM_TEST_TARGET videoSystemTest)
add_executable(${VIDEO_SYSTEM_TEST_TARGET}
    videoSystemTest.cpp)
target_link_libraries(${VIDEO_SYSTEM_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain VideoSystem ${SDL2_TARGET})
add_test(NAME ${VIDEO_SYSTEM_TEST_TARGET} COMMAND ${VIDEO_SYSTEM_TEST_TARGET})

set(MONITOR_TEST_TARGET monitorTest)
add_executable(${MONITOR_TEST_TARGET}
    monitorTest.cpp)
target_link_libraries(${MONITOR_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Graphics)
add_test(NAME ${MONITOR_TEST_TARGET} COMMAND ${MONITOR_TEST_TARGET})
