add_library(bitmapExtractor
    bitmapExtractor.cpp
    bitmapExtractor.h)
target_link_libraries(bitmapExtractor
    PRIVATE amosCompact binary bmpWriter gameData headers)
