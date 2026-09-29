add_library(States INTERFACE)

add_library(StatesShared
    shared/EngineStreetHost.cpp
    shared/EngineStreetHost.h
    shared/FotoScreen.cpp
    shared/FotoScreen.h
    shared/IntroStrip.cpp
    shared/IntroStrip.h
    shared/MusicFadeOut.cpp
    shared/MusicFadeOut.h
    shared/StageState.h
    shared/StreetControls.cpp
    shared/StreetControls.h
    shared/StreetVideo.cpp
    shared/StreetVideo.h)
target_link_libraries(StatesShared
    PUBLIC AudioSystem Common EffectsSequences Graphics Input States StreetCore
           StreetScenes StreetSession VideoSystem
    PRIVATE Assets StreetUi)

add_library(StatesAdverts
    adverts/AdvertsState.cpp
    adverts/AdvertsState.h)
target_link_libraries(StatesAdverts
    PUBLIC Common EffectsColor Graphics Input States VideoSystem
    PRIVATE Assets)

add_library(StatesCharacterSelection
    characterSelection/CharacterSelectionState.cpp
    characterSelection/CharacterSelectionState.h)
target_link_libraries(StatesCharacterSelection
    PUBLIC AudioSystem Common EffectsSequences Graphics Input States
           StreetSession VideoSystem
    PRIVATE Assets)

add_library(StatesContinueSelect
    continueSelect/ContinueState.cpp
    continueSelect/ContinueState.h)
target_link_libraries(StatesContinueSelect
    PUBLIC AudioSystem Common Input States StatesShared StreetScenes
           StreetSession VideoSystem)

add_library(StatesEnding
    ending/EndingState.cpp
    ending/EndingState.h)
target_link_libraries(StatesEnding
    PUBLIC AudioSystem Common Input States StatesShared StreetScenes
           StreetSession VideoSystem)

add_library(StatesGameOver
    gameOver/GameOverState.cpp
    gameOver/GameOverState.h)
target_link_libraries(StatesGameOver
    PUBLIC AudioSystem Common Input States StatesShared StreetScenes VideoSystem)

add_library(StatesHighScore
    highScore/HighScoreState.cpp
    highScore/HighScoreState.h)
target_link_libraries(StatesHighScore
    PUBLIC AudioSystem Common States StatesShared StreetScenes StreetSession
           VideoSystem)

add_library(StatesKneeAnimation
    kneeAnimation/KneeAnimationState.cpp
    kneeAnimation/KneeAnimationState.h)
target_link_libraries(StatesKneeAnimation
    PUBLIC AudioSystem Common Graphics Input States VideoSystem
    PRIVATE Assets EffectsColor)

add_library(StatesLevel1 INTERFACE)
target_link_libraries(StatesLevel1 INTERFACE StatesShared StreetScenes)

add_library(StatesLevel2 INTERFACE)
target_link_libraries(StatesLevel2 INTERFACE StatesShared StreetScenes)

add_library(StatesLevel3 INTERFACE)
target_link_libraries(StatesLevel3 INTERFACE StatesShared StreetScenes)

add_library(StatesMenu
    menu/MenuState.cpp
    menu/MenuState.h)
target_link_libraries(StatesMenu
    PUBLIC AudioSystem Common EffectsSequences Graphics Input States
           StreetSession VideoSystem
    PRIVATE Assets StreetScenes)

add_library(StatesMirage
    mirage/MirageState.cpp
    mirage/MirageState.h)
target_link_libraries(StatesMirage PUBLIC States StatesShared VideoSystem)

add_library(StatesPresents
    presents/PresentsState.cpp
    presents/PresentsState.h)
target_link_libraries(StatesPresents
    PUBLIC AudioSystem EffectsSequences Input States StatesShared VideoSystem)

add_library(StatesProtectionCheck
    protectionCheck/ProtectionCheckState.cpp
    protectionCheck/ProtectionCheckState.h)
target_link_libraries(StatesProtectionCheck
    PUBLIC AudioSystem Common EffectsColor EffectsProtection Graphics States
           VideoSystem
    PRIVATE StreetUi)

add_library(StatesSpiderLogo
    spiderLogo/SpiderLogoState.cpp
    spiderLogo/SpiderLogoState.h)
target_link_libraries(StatesSpiderLogo
    PUBLIC Amal AudioSystem Common EffectsSequences Graphics States VideoSystem
    PRIVATE Assets)

add_library(StatesTitleAndStory
    titleAndStory/TitleAndStoryState.cpp
    titleAndStory/TitleAndStoryState.h)
target_link_libraries(StatesTitleAndStory
    PUBLIC AudioSystem Common EffectsSequences Graphics Input States
           StatesShared VideoSystem
    PRIVATE Assets)

add_library(StatesWorldSoftware
    worldSoftware/WorldSoftwareState.cpp
    worldSoftware/WorldSoftwareState.h)
target_link_libraries(StatesWorldSoftware
    PUBLIC AudioSystem States StatesShared VideoSystem)
