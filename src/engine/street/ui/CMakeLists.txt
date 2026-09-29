add_library(StreetUi
    LoadingQueue.cpp
    LoadingQueue.h
    ScreenOutput.cpp
    ScreenOutput.h
    StageFrame.cpp
    StageFrame.h
    StatusPanel.cpp
    StatusPanel.h)
target_link_libraries(StreetUi
    PUBLIC Amal Common EffectsColor Graphics StreetCore
    PRIVATE StreetActors)
