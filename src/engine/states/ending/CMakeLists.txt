add_library(StatesEnding
    EndingState.cpp
    EndingState.h)
target_link_libraries(StatesEnding
    PUBLIC AudioSystem Common Input States StatesShared StreetScenes
           StreetSession VideoSystem)
