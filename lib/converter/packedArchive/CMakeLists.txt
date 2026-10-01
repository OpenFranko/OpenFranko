add_library(packedArchive
    lz4Compressor.cpp
    lz4Compressor.h
    packedArchive.cpp
    packedArchive.h)
target_link_libraries(packedArchive PUBLIC Graphics)
