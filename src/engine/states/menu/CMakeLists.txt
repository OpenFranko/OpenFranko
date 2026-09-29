add_library(StatesMenu
    MenuState.cpp
    MenuState.h)
target_link_libraries(StatesMenu
    PUBLIC AudioSystem Common EffectsSequences Graphics Input States
           StreetSession VideoSystem
    PRIVATE Assets StreetScenes)
