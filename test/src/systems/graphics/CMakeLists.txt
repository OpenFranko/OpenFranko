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
