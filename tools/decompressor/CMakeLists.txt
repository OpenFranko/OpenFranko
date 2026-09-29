add_executable(backwardLZ77Decompressor
    backwardLZ77Decompressor.cpp)
target_link_libraries(backwardLZ77Decompressor
    PRIVATE argumentParser backwardLZ77 fileContainer gameData readFile
            writeFile)
