add_library(StatesProtectionCheck
    ProtectionCheckState.cpp
    ProtectionCheckState.h)
target_link_libraries(StatesProtectionCheck
    PUBLIC AudioSystem Common EffectsColor EffectsProtection Graphics States
           VideoSystem
    PRIVATE StreetUi)
