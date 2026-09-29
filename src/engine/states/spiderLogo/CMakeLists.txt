add_library(StatesSpiderLogo
    SpiderLogoState.cpp
    SpiderLogoState.h)
target_link_libraries(StatesSpiderLogo
    PUBLIC Amal AudioSystem Common EffectsSequences Graphics States VideoSystem
    PRIVATE Assets)
