add_library(StatesPresents
    PresentsState.cpp
    PresentsState.h)
target_link_libraries(StatesPresents
    PUBLIC Assets Audio EffectsSequences Graphics Input States StatesShared)
