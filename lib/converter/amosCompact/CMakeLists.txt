add_library(amosCompact
    amosCompact.cpp
    amosCompact.h
    consts.h
    decodeAmosBitmap.cpp
    decodeAmosBitmap.h)
target_link_libraries(amosCompact PUBLIC headers PRIVATE binary bmpWriter)

add_subdirectory(detail)
