add_library(EffectsColor
    AmigaPalette.h
    PaletteFader.cpp
    PaletteFader.h
    PaletteFlasher.cpp
    PaletteFlasher.h
    Rainbow.cpp
    Rainbow.h)
target_link_libraries(EffectsColor PUBLIC Graphics)
