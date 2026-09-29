add_library(StatesHighScore
    HighScoreState.cpp
    HighScoreState.h)
target_link_libraries(StatesHighScore
    PUBLIC Common Graphics States StreetScenes StreetSession
    PRIVATE StatesShared)
