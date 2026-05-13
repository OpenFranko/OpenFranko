add_library(spriteSheet spriteSheet.cpp spriteSheet.h Palettes.h)
target_link_libraries(spriteSheet decodeImage bmpWriter)
