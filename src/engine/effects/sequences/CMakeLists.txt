add_library(EffectsSequences
    AttractSequence.cpp
    AttractSequence.h
    BlyskSequence.cpp
    BlyskSequence.h
    CharacterSelectionSequence.cpp
    CharacterSelectionSequence.h
    FotoSequence.cpp
    FotoSequence.h
    MenuSequence.cpp
    MenuSequence.h
    StorySequence.cpp
    StorySequence.h)
target_link_libraries(EffectsSequences
    PUBLIC Common EffectsAnimation EffectsColor)
