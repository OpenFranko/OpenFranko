add_library(StatesContinueSelect
    ContinueState.cpp
    ContinueState.h)
target_link_libraries(StatesContinueSelect
    PUBLIC Common Graphics Input States StreetScenes StreetSession
    PRIVATE StatesShared)
