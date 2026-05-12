add_library(decodeImage decodeImage.cpp decodeImage.h headers.cpp headers.h Palettes.h)
target_link_libraries(decodeImage amosCompact helpers)
