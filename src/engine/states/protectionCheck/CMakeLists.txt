add_library(StatesProtectionCheck
    ProtectionCheckState.cpp
    ProtectionCheckState.h)
target_link_libraries(StatesProtectionCheck
    PUBLIC Assets Audio Common EffectsColor EffectsProtection Graphics States
    PRIVATE StreetUi)
