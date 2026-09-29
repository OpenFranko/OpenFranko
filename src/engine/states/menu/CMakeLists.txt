add_library(StatesMenu
    MenuState.cpp
    MenuState.h)
target_link_libraries(StatesMenu
    PUBLIC Assets Audio Common EffectsSequences Graphics Input States
           StreetSession
    PRIVATE StreetScenes)
