add_library(fileContainer fileContainer.cpp fileContainer.h)
target_link_libraries(fileContainer binary backwardLZ77 gameData)
