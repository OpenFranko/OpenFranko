add_library(StatesWorldSoftware
    WorldSoftwareState.cpp
    WorldSoftwareState.h)
target_link_libraries(StatesWorldSoftware
    PUBLIC Assets Audio Graphics States StatesShared)
