set(ASSETS_TEST_TARGET assetsTest)
add_executable(${ASSETS_TEST_TARGET}
    assetsTest.cpp)
target_link_libraries(${ASSETS_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Assets)
add_test(NAME ${ASSETS_TEST_TARGET} COMMAND ${ASSETS_TEST_TARGET})

set(DISK_FILES_TEST_TARGET diskFilesTest)
add_executable(${DISK_FILES_TEST_TARGET}
    diskFilesTest.cpp)
target_link_libraries(${DISK_FILES_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Assets)
add_test(NAME ${DISK_FILES_TEST_TARGET} COMMAND ${DISK_FILES_TEST_TARGET})
