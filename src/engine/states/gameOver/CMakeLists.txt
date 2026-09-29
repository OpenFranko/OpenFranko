add_library(StatesGameOver
    GameOverState.cpp
    GameOverState.h)
target_link_libraries(StatesGameOver
    PUBLIC Common Graphics Input States StreetScenes
    PRIVATE StatesShared)
