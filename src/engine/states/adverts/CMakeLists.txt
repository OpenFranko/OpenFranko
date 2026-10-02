add_library(StatesAdverts
    AdvertsState.cpp
    AdvertsState.h)
target_link_libraries(StatesAdverts
    PUBLIC Assets Common EffectsColor Graphics Input States StatesShared)
