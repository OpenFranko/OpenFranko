add_library(StatesPresents
    PresentsState.cpp
    PresentsState.h)
target_link_libraries(StatesPresents
    PUBLIC AudioSystem EffectsSequences Input States StatesShared VideoSystem)
