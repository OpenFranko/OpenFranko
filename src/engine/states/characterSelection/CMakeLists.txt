add_library(StatesCharacterSelection
    CharacterSelectionState.cpp
    CharacterSelectionState.h)
target_link_libraries(StatesCharacterSelection
    PUBLIC Assets Audio Common EffectsSequences Graphics Input States
           StreetSession)
