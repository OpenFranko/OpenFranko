set(ABK_TO_S3M_TEST_TARGET abkToS3mTest)
add_executable(${ABK_TO_S3M_TEST_TARGET}
    abkToS3mTest.cpp)
target_link_libraries(${ABK_TO_S3M_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain abkToS3m binary)
add_test(NAME ${ABK_TO_S3M_TEST_TARGET} COMMAND ${ABK_TO_S3M_TEST_TARGET})

set(AMOS_COMPACT_TEST_TARGET amosCompactTest)
add_executable(${AMOS_COMPACT_TEST_TARGET}
    amosCompactTest.cpp)
target_link_libraries(${AMOS_COMPACT_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain amosCompact binary)
add_test(NAME ${AMOS_COMPACT_TEST_TARGET} COMMAND ${AMOS_COMPACT_TEST_TARGET})

set(AUDIO_EXTRACTOR_TEST_TARGET audioExtractorTest)
add_executable(${AUDIO_EXTRACTOR_TEST_TARGET}
    audioExtractorTest.cpp)
target_link_libraries(${AUDIO_EXTRACTOR_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain audioExtractor binary)
add_test(NAME ${AUDIO_EXTRACTOR_TEST_TARGET}
    COMMAND ${AUDIO_EXTRACTOR_TEST_TARGET})

set(BITMAP_EXTRACTOR_TEST_TARGET bitmapExtractorTest)
add_executable(${BITMAP_EXTRACTOR_TEST_TARGET}
    bitmapExtractorTest.cpp)
target_link_libraries(${BITMAP_EXTRACTOR_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain binary bitmapExtractor)
add_test(NAME ${BITMAP_EXTRACTOR_TEST_TARGET}
    COMMAND ${BITMAP_EXTRACTOR_TEST_TARGET})

set(CODE_CARDS_TEST_TARGET codeCardsTest)
add_executable(${CODE_CARDS_TEST_TARGET}
    codeCardsTest.cpp)
target_link_libraries(${CODE_CARDS_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain codeCards)
add_test(NAME ${CODE_CARDS_TEST_TARGET} COMMAND ${CODE_CARDS_TEST_TARGET})

set(ENDING_CREDITS_TEST_TARGET endingCreditsTest)
add_executable(${ENDING_CREDITS_TEST_TARGET}
    endingCreditsTest.cpp)
target_link_libraries(${ENDING_CREDITS_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain binary endingCredits)
add_test(NAME ${ENDING_CREDITS_TEST_TARGET}
    COMMAND ${ENDING_CREDITS_TEST_TARGET})

set(FILE_CONTAINER_TEST_TARGET fileContainerTest)
add_executable(${FILE_CONTAINER_TEST_TARGET}
    fileContainerTest.cpp)
target_link_libraries(${FILE_CONTAINER_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain binary fileContainer gameData)
add_test(NAME ${FILE_CONTAINER_TEST_TARGET}
    COMMAND ${FILE_CONTAINER_TEST_TARGET})

set(GAME_DATA_TEST_TARGET gameDataTest)
add_executable(${GAME_DATA_TEST_TARGET}
    gameDataTest.cpp)
target_link_libraries(${GAME_DATA_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain gameData)
add_test(NAME ${GAME_DATA_TEST_TARGET} COMMAND ${GAME_DATA_TEST_TARGET})

set(HEADERS_TEST_TARGET headersTest)
add_executable(${HEADERS_TEST_TARGET}
    headersTest.cpp)
target_link_libraries(${HEADERS_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain headers)
add_test(NAME ${HEADERS_TEST_TARGET} COMMAND ${HEADERS_TEST_TARGET})

set(JAGUAR_CARTRIDGE_TEST_TARGET jaguarCartridgeTest)
add_executable(${JAGUAR_CARTRIDGE_TEST_TARGET}
    jaguarCartridgeTest.cpp)
target_link_libraries(${JAGUAR_CARTRIDGE_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain jaguarCartridge)
add_test(NAME ${JAGUAR_CARTRIDGE_TEST_TARGET}
    COMMAND ${JAGUAR_CARTRIDGE_TEST_TARGET})

set(LEVEL_SCRIPT_TEST_TARGET levelScriptTest)
add_executable(${LEVEL_SCRIPT_TEST_TARGET}
    levelScriptTest.cpp)
target_link_libraries(${LEVEL_SCRIPT_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain binary levelScript)
add_test(NAME ${LEVEL_SCRIPT_TEST_TARGET} COMMAND ${LEVEL_SCRIPT_TEST_TARGET})

set(SPRITE_SHEET_TEST_TARGET spriteSheetTest)
add_executable(${SPRITE_SHEET_TEST_TARGET}
    spriteSheetTest.cpp)
target_link_libraries(${SPRITE_SHEET_TEST_TARGET}
    PRIVATE Catch2::Catch2WithMain binary gameData spriteSheet)
add_test(NAME ${SPRITE_SHEET_TEST_TARGET} COMMAND ${SPRITE_SHEET_TEST_TARGET})
