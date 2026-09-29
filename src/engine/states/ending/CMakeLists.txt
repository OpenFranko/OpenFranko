add_library(StatesEnding
    EndingState.cpp
    EndingState.h)
target_link_libraries(StatesEnding
    PUBLIC Common Graphics Input States StreetScenes StreetSession
    PRIVATE StatesShared)
