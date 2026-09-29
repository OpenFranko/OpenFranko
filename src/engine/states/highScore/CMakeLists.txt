add_library(StatesHighScore
    HighScoreState.cpp
    HighScoreState.h)
target_link_libraries(StatesHighScore
    PUBLIC AudioSystem Common States StatesShared StreetScenes StreetSession
           VideoSystem)
