add_library(StatesKneeAnimation
    KneeAnimationState.cpp
    KneeAnimationState.h)
target_link_libraries(StatesKneeAnimation
    PUBLIC Assets Audio Common Graphics Input States StatesShared
    PRIVATE EffectsColor)
