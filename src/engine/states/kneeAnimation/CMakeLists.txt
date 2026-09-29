add_library(StatesKneeAnimation
    KneeAnimationState.cpp
    KneeAnimationState.h)
target_link_libraries(StatesKneeAnimation
    PUBLIC AudioSystem Common Graphics Input States VideoSystem
    PRIVATE Assets EffectsColor)
