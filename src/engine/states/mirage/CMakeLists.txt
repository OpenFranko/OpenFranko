add_library(StatesMirage
    MirageState.cpp
    MirageState.h)
target_link_libraries(StatesMirage PUBLIC States StatesShared VideoSystem)
