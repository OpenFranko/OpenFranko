add_library(StreetScenes
    BossStage.cpp
    BossStage.h
    CarStage.cpp
    CarStage.h
    ContinueScene.cpp
    ContinueScene.h
    EndingScene.cpp
    EndingScene.h
    GameOverScene.cpp
    GameOverScene.h
    HighScoreScene.cpp
    HighScoreScene.h
    Stage.cpp
    Stage.h
    StreetHost.h
    StreetStage.cpp
    StreetStage.h)
target_link_libraries(StreetScenes
    PUBLIC Amal Common EffectsAnimation EffectsColor Graphics StreetCore
           StreetSession StreetUi
    PRIVATE Assets Audio EffectsSequences Input StreetActors
            StreetActorsCompiled)
