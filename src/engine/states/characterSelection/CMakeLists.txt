add_library(StatesCharacterSelection
    CharacterSelectionState.cpp
    CharacterSelectionState.h)
target_link_libraries(StatesCharacterSelection
    PUBLIC AudioSystem Common EffectsSequences Graphics Input States
           StreetSession VideoSystem
    PRIVATE Assets)
