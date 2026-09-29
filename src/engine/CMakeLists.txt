add_subdirectory(amal)
add_subdirectory(assets)
add_subdirectory(effects)
add_subdirectory(states)
add_subdirectory(street)

add_library(Common
    AmigaDisplay.h
    GameOptions.h
    GameVersion.h
    InkeyBuffer.cpp
    InkeyBuffer.h
    MenuTempo.h)

add_library(Engine
    Engine.cpp
    Engine.h)
target_link_libraries(Engine
    PUBLIC AudioSystem Common Input Platform States StreetSession VideoSystem
    PRIVATE Assets StatesAdverts StatesCharacterSelection StatesContinueSelect
            StatesEnding StatesGameOver StatesHighScore StatesKneeAnimation
            StatesLevel1 StatesLevel2 StatesLevel3 StatesMenu StatesMirage
            StatesPresents StatesProtectionCheck StatesSpiderLogo
            StatesTitleAndStory StatesWorldSoftware)

if(SKIP_COPY_PROTECTION)
    target_compile_definitions(Engine PRIVATE SKIP_COPY_PROTECTION)
endif()
