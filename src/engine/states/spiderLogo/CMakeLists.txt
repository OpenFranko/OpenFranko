add_library(StatesSpiderLogo
    SpiderLogoState.cpp
    SpiderLogoState.h)
target_link_libraries(StatesSpiderLogo
    PUBLIC Amal Assets Audio Common EffectsSequences Graphics States)
