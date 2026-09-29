add_library(spriteSheet spriteSheet.cpp spriteSheet.h Palettes.h)
target_link_libraries(spriteSheet amosCompact shared bmpWriter gameData)
