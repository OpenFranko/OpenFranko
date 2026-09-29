add_library(spriteSheet
    spriteSheet.cpp
    spriteSheet.h)
target_link_libraries(spriteSheet
    PUBLIC gameData
    PRIVATE amosCompact binary bmpWriter headers)
