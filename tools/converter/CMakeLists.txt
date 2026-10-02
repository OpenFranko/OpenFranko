add_subdirectory(frankoResourceExtractor)

add_executable(convertAbkToS3m
    convertAbkToS3m.cpp)
target_link_libraries(convertAbkToS3m
    PRIVATE abkToS3m argumentParser readFile writeFile)

add_executable(extractBitmaps
    extractBitmaps.cpp)
target_link_libraries(extractBitmaps
    PRIVATE argumentParser bitmapExtractor fileContainer gameData readFile
            writeFile)

add_executable(extractCodeCards
    extractCodeCards.cpp)
target_link_libraries(extractCodeCards
    PRIVATE argumentParser codeCards fileContainer gameData readFile writeFile)

add_executable(extractCredits
    extractCredits.cpp)
target_link_libraries(extractCredits
    PRIVATE argumentParser endingCredits readFile writeFile)

add_executable(extractLevelScript
    extractLevelScript.cpp)
target_link_libraries(extractLevelScript
    PRIVATE argumentParser fileContainer gameData levelScript readFile
            writeFile)

add_executable(extractMusic
    extractMusic.cpp)
target_link_libraries(extractMusic
    PRIVATE argumentParser audioExtractor fileContainer gameData readFile
            writeFile)

add_executable(extractSamples
    extractSamples.cpp)
target_link_libraries(extractSamples
    PRIVATE argumentParser audioExtractor fileContainer gameData readFile
            writeFile)

add_executable(extractScreen
    extractScreen.cpp)
target_link_libraries(extractScreen
    PRIVATE amosCompact argumentParser fileContainer gameData readFile
            writeFile)

add_executable(extractSpriteSheet
    extractSpriteSheet.cpp)
target_link_libraries(extractSpriteSheet
    PRIVATE argumentParser fileContainer gameData readFile spriteSheet
            writeFile)

add_executable(extractSprites
    extractSprites.cpp)
target_link_libraries(extractSprites
    PRIVATE argumentParser fileContainer gameData readFile spriteSheet
            writeFile)

add_executable(packAssets
    packAssets.cpp)
target_link_libraries(packAssets PRIVATE argumentParser packedArchive)

add_executable(makeCartridge
    makeCartridge.cpp)
target_link_libraries(makeCartridge
    PRIVATE argumentParser jaguarCartridge packedArchive)

add_executable(frankoExtract
    frankoExtract.cpp)
target_link_libraries(frankoExtract
    PRIVATE argumentParser frankoResourceExtractor)
