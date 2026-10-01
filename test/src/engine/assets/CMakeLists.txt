set(ARCHIVE_FILES_TEST_TARGET archiveFilesTest)
add_executable(${ARCHIVE_FILES_TEST_TARGET}
    archiveFilesTest.cpp)
target_link_libraries(${ARCHIVE_FILES_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Assets)
add_test(NAME ${ARCHIVE_FILES_TEST_TARGET} COMMAND ${ARCHIVE_FILES_TEST_TARGET})

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

set(REQUIRED_FILES_TEST_TARGET requiredFilesTest)
add_executable(${REQUIRED_FILES_TEST_TARGET}
    requiredFilesTest.cpp)
target_link_libraries(${REQUIRED_FILES_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Assets)
add_test(NAME ${REQUIRED_FILES_TEST_TARGET}
    COMMAND ${REQUIRED_FILES_TEST_TARGET})

set(YIELDING_FILES_TEST_TARGET yieldingFilesTest)
add_executable(${YIELDING_FILES_TEST_TARGET}
    yieldingFilesTest.cpp)
target_link_libraries(${YIELDING_FILES_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Assets)
add_test(NAME ${YIELDING_FILES_TEST_TARGET}
    COMMAND ${YIELDING_FILES_TEST_TARGET})

set(PACKED_FILES_TEST_TARGET packedFilesTest)
add_executable(${PACKED_FILES_TEST_TARGET}
    packedFilesTest.cpp)
target_link_libraries(${PACKED_FILES_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain Assets packedArchive)
add_test(NAME ${PACKED_FILES_TEST_TARGET} COMMAND ${PACKED_FILES_TEST_TARGET})
