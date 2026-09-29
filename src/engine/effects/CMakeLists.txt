add_library(EffectsAnimation
    animation/AmalAnim.cpp
    animation/AmalAnim.h
    animation/AmalMotion.cpp
    animation/AmalMotion.h
    animation/Bob.h
    animation/CreditScroll.cpp
    animation/CreditScroll.h)

add_library(EffectsColor
    color/AmigaPalette.h
    color/PaletteFader.cpp
    color/PaletteFader.h
    color/PaletteFlasher.cpp
    color/PaletteFlasher.h
    color/Rainbow.cpp
    color/Rainbow.h)

add_library(EffectsProtection
    protection/CodeCardCheck.cpp
    protection/CodeCardCheck.h)

add_library(EffectsSequences
    sequences/AttractSequence.cpp
    sequences/AttractSequence.h
    sequences/BlyskSequence.cpp
    sequences/BlyskSequence.h
    sequences/CharacterSelectionSequence.cpp
    sequences/CharacterSelectionSequence.h
    sequences/FotoSequence.cpp
    sequences/FotoSequence.h
    sequences/MenuSequence.cpp
    sequences/MenuSequence.h
    sequences/StorySequence.cpp
    sequences/StorySequence.h)
target_link_libraries(EffectsSequences
    PUBLIC Common EffectsAnimation EffectsColor)
