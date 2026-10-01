set(BOBS_TEST_TARGET bobsTest)
add_executable(${BOBS_TEST_TARGET}
    bobsTest.cpp)
target_link_libraries(${BOBS_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain StreetCore)
add_test(NAME ${BOBS_TEST_TARGET} COMMAND ${BOBS_TEST_TARGET})

set(DOUBLE_BUFFER_TEST_TARGET doubleBufferTest)
add_executable(${DOUBLE_BUFFER_TEST_TARGET}
    doubleBufferTest.cpp)
target_link_libraries(${DOUBLE_BUFFER_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain StreetCore)
add_test(NAME ${DOUBLE_BUFFER_TEST_TARGET} COMMAND ${DOUBLE_BUFFER_TEST_TARGET})

set(HIGH_SCORE_TABLE_TEST_TARGET highScoreTableTest)
add_executable(${HIGH_SCORE_TABLE_TEST_TARGET}
    highScoreTableTest.cpp)
target_link_libraries(${HIGH_SCORE_TABLE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain StreetCore)
add_test(NAME ${HIGH_SCORE_TABLE_TEST_TARGET}
    COMMAND ${HIGH_SCORE_TABLE_TEST_TARGET})

set(INDEXED_SURFACE_TEST_TARGET indexedSurfaceTest)
add_executable(${INDEXED_SURFACE_TEST_TARGET}
    indexedSurfaceTest.cpp)
target_link_libraries(${INDEXED_SURFACE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain StreetCore)
add_test(NAME ${INDEXED_SURFACE_TEST_TARGET}
    COMMAND ${INDEXED_SURFACE_TEST_TARGET})

set(STREET_JSON_TEST_TARGET streetJsonTest)
add_executable(${STREET_JSON_TEST_TARGET}
    streetJsonTest.cpp)
target_link_libraries(${STREET_JSON_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain StreetCore)
add_test(NAME ${STREET_JSON_TEST_TARGET} COMMAND ${STREET_JSON_TEST_TARGET})

set(STREET_LEVEL_SCRIPT_TEST_TARGET streetLevelScriptTest)
add_executable(${STREET_LEVEL_SCRIPT_TEST_TARGET}
    streetLevelScriptTest.cpp)
target_link_libraries(${STREET_LEVEL_SCRIPT_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain StreetCore)
add_test(NAME ${STREET_LEVEL_SCRIPT_TEST_TARGET}
    COMMAND ${STREET_LEVEL_SCRIPT_TEST_TARGET})

set(HIGH_SCORE_STORAGE_TEST_TARGET highScoreStorageTest)
add_executable(${HIGH_SCORE_STORAGE_TEST_TARGET}
    highScoreStorageTest.cpp)
target_link_libraries(${HIGH_SCORE_STORAGE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain StreetCore)
add_test(NAME ${HIGH_SCORE_STORAGE_TEST_TARGET}
    COMMAND ${HIGH_SCORE_STORAGE_TEST_TARGET})

set(SURFACE_PAIR_TEST_TARGET surfacePairTest)
add_executable(${SURFACE_PAIR_TEST_TARGET}
    surfacePairTest.cpp)
target_link_libraries(${SURFACE_PAIR_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain StreetCore)
add_test(NAME ${SURFACE_PAIR_TEST_TARGET} COMMAND ${SURFACE_PAIR_TEST_TARGET})
