add_library(frankoResourceExtractor
    frankoResourceExtractor.cpp
    frankoResourceExtractor.h)
target_link_libraries(frankoResourceExtractor
    PRIVATE abkToS3m amosCompact audioExtractor binary bitmapExtractor
            codeCards endingCredits fileContainer gameData levelScript readFile
            spriteSheet writeFile)
