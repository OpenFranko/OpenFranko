add_executable(compileActorPrograms
    compileActorPrograms.cpp)
target_link_libraries(compileActorPrograms
    PRIVATE Amal argumentParser Common StreetActors writeFile)
