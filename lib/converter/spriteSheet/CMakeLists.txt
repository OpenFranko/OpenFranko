add_library(spriteSheet
    spriteSheet.cpp
    spriteSheet.h)
target_link_libraries(spriteSheet
    PRIVATE amosCompact binary bmpWriter gameData headers)
