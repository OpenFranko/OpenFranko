add_library(StatesAdverts
    AdvertsState.cpp
    AdvertsState.h)
target_link_libraries(StatesAdverts
    PUBLIC Common EffectsColor Graphics Input States VideoSystem
    PRIVATE Assets)
