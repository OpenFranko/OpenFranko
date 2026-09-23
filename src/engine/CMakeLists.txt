add_library(Effects effects/PaletteFader.cpp effects/PaletteFader.h effects/FotoSequence.cpp effects/FotoSequence.h)

add_library(Engine Engine.cpp Engine.h states/mirage/MirageState.cpp states/mirage/MirageState.h states/worldSoftware/WorldSoftwareState.cpp states/worldSoftware/WorldSoftwareState.h)
target_link_libraries(Engine PRIVATE VideoSystem AudioSystem ControllerSystem Effects)