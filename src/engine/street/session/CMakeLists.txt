add_library(StreetSession
    CheatCodes.cpp
    CheatCodes.h
    GameSession.cpp
    GameSession.h)
target_link_libraries(StreetSession
    PUBLIC Amal Common EffectsColor StreetCore StreetUi)
