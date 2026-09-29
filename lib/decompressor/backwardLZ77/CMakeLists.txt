add_library(backwardLZ77
    BitReader.cpp
    BitReader.h
    backwardLZ77.cpp
    backwardLZ77.h
    consts.h)
target_link_libraries(backwardLZ77 PRIVATE binary)
