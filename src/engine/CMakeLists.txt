add_library(Effects
    effects/color/AmigaPalette.h
    effects/color/AmigaDisplay.h
    effects/color/PaletteFader.cpp
    effects/color/PaletteFader.h
    effects/color/PaletteFlasher.cpp
    effects/color/PaletteFlasher.h
    effects/color/Rainbow.cpp
    effects/color/Rainbow.h
    effects/sequences/FotoSequence.cpp
    effects/sequences/FotoSequence.h
    effects/sequences/StorySequence.cpp
    effects/sequences/StorySequence.h
    effects/sequences/CodeCardCheck.cpp
    effects/sequences/CodeCardCheck.h
    effects/core/GameOptions.h
    effects/animation/AmalMotion.cpp
    effects/animation/AmalMotion.h
    effects/sequences/CreditScroll.cpp
    effects/sequences/CreditScroll.h
    effects/sequences/MenuSequence.cpp
    effects/sequences/MenuSequence.h
    effects/core/InkeyBuffer.cpp
    effects/core/InkeyBuffer.h
    effects/sequences/AttractSequence.cpp
    effects/sequences/AttractSequence.h
    effects/animation/AmalAnim.cpp
    effects/animation/AmalAnim.h
    effects/animation/CharacterSelection.cpp
    effects/animation/CharacterSelection.h
    effects/sequences/BlyskSequence.cpp
    effects/sequences/BlyskSequence.h)

target_include_directories(Effects PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/effects
    ${CMAKE_CURRENT_SOURCE_DIR}/effects/animation
    ${CMAKE_CURRENT_SOURCE_DIR}/effects/color
    ${CMAKE_CURRENT_SOURCE_DIR}/effects/core
    ${CMAKE_CURRENT_SOURCE_DIR}/effects/sequences)

add_library(Assets assets/Assets.cpp assets/Assets.h GameVersion.h)

add_library(Amal amal/Program.cpp amal/Program.h amal/Machine.cpp amal/Machine.h amal/Actors.cpp amal/Actors.h)

add_library(Street street/core/IndexedSurface.cpp street/core/IndexedSurface.h street/core/Bobs.cpp street/core/Bobs.h street/ui/StatusPanel.cpp street/ui/StatusPanel.h street/core/LevelScript.cpp street/core/LevelScript.h street/core/Json.cpp street/core/Json.h street/scenes/EndingCredits.cpp street/scenes/EndingCredits.h street/ui/GameSession.cpp street/ui/GameSession.h street/core/CheatCodes.cpp street/core/CheatCodes.h street/core/LoadingMock.cpp street/core/LoadingMock.h street/core/StageFrame.cpp street/core/StageFrame.h street/scenes/StreetStage.cpp street/scenes/StreetStage.h street/scenes/BossStage.cpp street/scenes/BossStage.h street/scenes/GameOverScene.cpp street/scenes/GameOverScene.h street/scenes/HighScoreTable.cpp street/scenes/HighScoreTable.h street/scenes/HighScoreScene.cpp street/scenes/HighScoreScene.h street/scenes/ContinueScene.cpp street/scenes/ContinueScene.h street/core/SystemText.cpp street/core/SystemText.h street/scenes/CarStage.cpp street/scenes/CarStage.h street/scenes/EndingScene.cpp street/scenes/EndingScene.h street/core/DoubleBuffer.cpp street/core/DoubleBuffer.h)
target_link_libraries(Street PUBLIC Amal Effects Graphics)

add_library(Engine Engine.cpp Engine.h states/mirage/MirageState.cpp states/mirage/MirageState.h states/worldSoftware/WorldSoftwareState.cpp states/worldSoftware/WorldSoftwareState.h states/kneeAnimation/KneeAnimationState.cpp states/kneeAnimation/KneeAnimationState.h states/titleAndStory/TitleAndStoryState.cpp states/titleAndStory/TitleAndStoryState.h states/protectionCheck/ProtectionCheckState.cpp states/protectionCheck/ProtectionCheckState.h states/menu/MenuState.cpp states/menu/MenuState.h states/characterSelection/CharacterSelectionState.cpp states/characterSelection/CharacterSelectionState.h states/shared/EngineStreetHost.cpp states/shared/EngineStreetHost.h states/shared/StreetControls.cpp states/shared/StreetControls.h states/level1/Level1State.cpp states/level1/Level1State.h states/level1/Level1BossState.cpp states/level1/Level1BossState.h states/level1/Level1CarState.cpp states/level1/Level1CarState.h states/level2/Level2State.cpp states/level2/Level2State.h states/level2/Level2BossState.cpp states/level2/Level2BossState.h states/level2/Level2CarState.cpp states/level2/Level2CarState.h states/level3/Level3State.cpp states/level3/Level3State.h states/level3/Level3BossState.cpp states/level3/Level3BossState.h states/ending/EndingState.cpp states/ending/EndingState.h states/gameOver/GameOverState.cpp states/gameOver/GameOverState.h states/highScore/HighScoreState.cpp states/highScore/HighScoreState.h states/continueSelect/ContinueState.cpp states/continueSelect/ContinueState.h states/spiderLogo/SpiderLogoState.cpp states/spiderLogo/SpiderLogoState.h states/adverts/AdvertsState.cpp states/adverts/AdvertsState.h states/shared/IntroStrip.cpp states/shared/IntroStrip.h states/shared/MusicFadeOut.cpp states/shared/MusicFadeOut.h states/presents/PresentsState.cpp states/presents/PresentsState.h)
target_link_libraries(Engine PRIVATE Platform VideoSystem Graphics AudioSystem ControllerSystem Effects Street Assets)

if(SKIP_COPY_PROTECTION)
    target_compile_definitions(Engine PRIVATE SKIP_COPY_PROTECTION)
endif()