find_package(Catch2 REQUIRED)

set(ARGUMENT_PARSER_TEST_TARGET argumentParserTest)
add_executable(${ARGUMENT_PARSER_TEST_TARGET}
    argumentParser/argumentParserTest.cpp
)
target_link_libraries(${ARGUMENT_PARSER_TEST_TARGET} PRIVATE Catch2::Catch2WithMain argumentParser)
add_test(NAME ${ARGUMENT_PARSER_TEST_TARGET} COMMAND ${ARGUMENT_PARSER_TEST_TARGET})

set(READ_FILE_TEST_TARGET readFileTest)
add_executable(${READ_FILE_TEST_TARGET}
    filesystem/readFileTest.cpp
)
target_link_libraries(${READ_FILE_TEST_TARGET} PRIVATE Catch2::Catch2WithMain readFile)
add_test(NAME ${READ_FILE_TEST_TARGET} COMMAND ${READ_FILE_TEST_TARGET})

set(WRITE_FILE_TEST_TARGET writeFileTest)
add_executable(${WRITE_FILE_TEST_TARGET}
    filesystem/writeFileTest.cpp
)
target_link_libraries(${WRITE_FILE_TEST_TARGET} PRIVATE Catch2::Catch2WithMain writeFile readFile)
add_test(NAME ${WRITE_FILE_TEST_TARGET} COMMAND ${WRITE_FILE_TEST_TARGET})

set(BACKWARD_LZ77_TEST_TARGET backwardLZ77Test)
add_executable(${BACKWARD_LZ77_TEST_TARGET}
    decompressor/backwardLZ77Test.cpp
)
target_link_libraries(${BACKWARD_LZ77_TEST_TARGET} PRIVATE Catch2::Catch2WithMain backwardLZ77)
add_test(NAME ${BACKWARD_LZ77_TEST_TARGET} COMMAND ${BACKWARD_LZ77_TEST_TARGET})

set(AMOS_COMPACT_TEST_TARGET amosCompactTest)
add_executable(${AMOS_COMPACT_TEST_TARGET}
    converter/amosCompactTest.cpp
)
target_link_libraries(${AMOS_COMPACT_TEST_TARGET} PRIVATE Catch2::Catch2WithMain amosCompact)
add_test(NAME ${AMOS_COMPACT_TEST_TARGET} COMMAND ${AMOS_COMPACT_TEST_TARGET})

set(BMP_WRITER_TEST_TARGET bmpWriterTest)
add_executable(${BMP_WRITER_TEST_TARGET}
    bmpWriter/bmpWriterTest.cpp
)
target_link_libraries(${BMP_WRITER_TEST_TARGET} PRIVATE Catch2::Catch2WithMain bmpWriter)
add_test(NAME ${BMP_WRITER_TEST_TARGET} COMMAND ${BMP_WRITER_TEST_TARGET})

set(FILE_CONTAINER_TEST_TARGET fileContainerTest)
add_executable(${FILE_CONTAINER_TEST_TARGET}
    converter/fileContainerTest.cpp
)
target_link_libraries(${FILE_CONTAINER_TEST_TARGET} PRIVATE Catch2::Catch2WithMain fileContainer)
add_test(NAME ${FILE_CONTAINER_TEST_TARGET} COMMAND ${FILE_CONTAINER_TEST_TARGET})

set(SPRITE_SHEET_TEST_TARGET spriteSheetTest)
add_executable(${SPRITE_SHEET_TEST_TARGET}
    converter/spriteSheetTest.cpp
)
target_link_libraries(${SPRITE_SHEET_TEST_TARGET} PRIVATE Catch2::Catch2WithMain spriteSheet)
add_test(NAME ${SPRITE_SHEET_TEST_TARGET} COMMAND ${SPRITE_SHEET_TEST_TARGET})

set(BITMAP_EXTRACTOR_TEST_TARGET bitmapExtractorTest)
add_executable(${BITMAP_EXTRACTOR_TEST_TARGET}
    converter/bitmapExtractorTest.cpp
)
target_link_libraries(${BITMAP_EXTRACTOR_TEST_TARGET} PRIVATE Catch2::Catch2WithMain bitmapExtractor)
add_test(NAME ${BITMAP_EXTRACTOR_TEST_TARGET} COMMAND ${BITMAP_EXTRACTOR_TEST_TARGET})