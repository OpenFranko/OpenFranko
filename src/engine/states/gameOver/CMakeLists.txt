add_library(StatesGameOver
    GameOverState.cpp
    GameOverState.h)
target_link_libraries(StatesGameOver
    PUBLIC AudioSystem Common Input States StatesShared StreetScenes VideoSystem)
