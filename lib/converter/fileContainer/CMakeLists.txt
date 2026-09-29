add_library(fileContainer
    fileContainer.cpp
    fileContainer.h)
target_link_libraries(fileContainer
    PRIVATE backwardLZ77 binary gameData headers)
