add_library(StatesWorldSoftware
    WorldSoftwareState.cpp
    WorldSoftwareState.h)
target_link_libraries(StatesWorldSoftware
    PUBLIC AudioSystem States StatesShared VideoSystem)
