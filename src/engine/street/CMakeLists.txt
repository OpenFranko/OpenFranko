add_library(StreetActors
    actors/Actors.cpp
    actors/Actors.h)
target_link_libraries(StreetActors PUBLIC Common)

add_library(StreetCore
    core/Bobs.cpp
    core/Bobs.h
    core/DoubleBuffer.cpp
    core/DoubleBuffer.h
    core/EndingCredits.cpp
    core/EndingCredits.h
    core/Font.cpp
    core/Font.h
    core/HighScoreTable.cpp
    core/HighScoreTable.h
    core/IndexedSurface.cpp
    core/IndexedSurface.h
    core/Json.cpp
    core/Json.h
    core/LevelScript.cpp
    core/LevelScript.h
    core/SystemText.cpp
    core/SystemText.h
    core/UpdateHold.cpp
    core/UpdateHold.h)
target_link_libraries(StreetCore PUBLIC Amal Common)

add_library(StreetUi
    ui/LoadingQueue.cpp
    ui/LoadingQueue.h
    ui/ScreenOutput.cpp
    ui/ScreenOutput.h
    ui/StageFrame.cpp
    ui/StageFrame.h
    ui/StatusPanel.cpp
    ui/StatusPanel.h)
target_link_libraries(StreetUi
    PUBLIC Amal Common EffectsColor Graphics StreetCore
    PRIVATE StreetActors)

add_library(StreetSession
    session/CheatCodes.cpp
    session/CheatCodes.h
    session/GameSession.cpp
    session/GameSession.h)
target_link_libraries(StreetSession
    PUBLIC Amal Common EffectsColor StreetCore StreetUi)

add_library(StreetScenes
    scenes/BossStage.cpp
    scenes/BossStage.h
    scenes/CarStage.cpp
    scenes/CarStage.h
    scenes/ContinueScene.cpp
    scenes/ContinueScene.h
    scenes/EndingScene.cpp
    scenes/EndingScene.h
    scenes/GameOverScene.cpp
    scenes/GameOverScene.h
    scenes/HighScoreScene.cpp
    scenes/HighScoreScene.h
    scenes/Stage.cpp
    scenes/Stage.h
    scenes/StreetHost.h
    scenes/StreetStage.cpp
    scenes/StreetStage.h)
target_link_libraries(StreetScenes
    PUBLIC Amal Common EffectsAnimation EffectsColor Graphics StreetCore
           StreetSession StreetUi
    PRIVATE StreetActors)
