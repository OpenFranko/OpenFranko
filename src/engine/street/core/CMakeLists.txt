add_library(StreetCore
    Bobs.cpp
    Bobs.h
    DoubleBuffer.cpp
    DoubleBuffer.h
    EndingCredits.cpp
    EndingCredits.h
    Font.cpp
    Font.h
    HighScoreStorage.cpp
    HighScoreStorage.h
    HighScoreTable.cpp
    HighScoreTable.h
    IndexedSurface.cpp
    IndexedSurface.h
    Json.cpp
    Json.h
    LevelScript.cpp
    LevelScript.h
    SurfacePair.cpp
    SurfacePair.h
    SystemText.cpp
    SystemText.h
    UpdateHold.cpp
    UpdateHold.h)
target_link_libraries(StreetCore PUBLIC Amal Common Graphics)

if(JAGUAR)
    target_sources(StreetCore PRIVATE JaguarHighScoreFile.cpp)
    target_link_libraries(StreetCore PRIVATE JaguarHardware)
else()
    target_sources(StreetCore PRIVATE HighScoreFile.cpp)
endif()
