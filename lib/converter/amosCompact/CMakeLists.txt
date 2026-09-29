add_library(amosCompact amosCompact.cpp amosCompact.h decodeAmosBitmap.cpp decodeAmosBitmap.h)
target_link_libraries(amosCompact binary bmpWriter headers)
add_subdirectory(detail)
