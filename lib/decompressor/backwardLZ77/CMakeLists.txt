add_library(backwardLZ77
    backwardLZ77.cpp
    backwardLZ77.h
    consts.h)
target_link_libraries(backwardLZ77 PRIVATE binary)

add_subdirectory(detail)
