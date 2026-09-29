add_library(StatesShared
    EngineStreetHost.cpp
    EngineStreetHost.h
    FotoScreen.cpp
    FotoScreen.h
    IntroStrip.cpp
    IntroStrip.h
    MusicFadeOut.cpp
    MusicFadeOut.h
    StageState.h
    StreetControls.cpp
    StreetControls.h
    StreetVideo.cpp
    StreetVideo.h)
target_link_libraries(StatesShared
    PUBLIC Assets Audio Common EffectsSequences Graphics Input States StreetCore
           StreetScenes StreetSession
    PRIVATE StreetUi)
