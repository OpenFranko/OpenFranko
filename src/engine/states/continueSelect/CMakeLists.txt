add_library(StatesContinueSelect
    ContinueState.cpp
    ContinueState.h)
target_link_libraries(StatesContinueSelect
    PUBLIC AudioSystem Common Input States StatesShared StreetScenes
           StreetSession VideoSystem)
