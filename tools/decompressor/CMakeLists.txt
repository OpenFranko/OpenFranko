add_executable(decompressBackwardLZ77
    decompressBackwardLZ77.cpp)
target_link_libraries(decompressBackwardLZ77
    PRIVATE argumentParser backwardLZ77 fileContainer gameData readFile
            writeFile)
