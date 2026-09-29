add_library(StatesTitleAndStory
    TitleAndStoryState.cpp
    TitleAndStoryState.h)
target_link_libraries(StatesTitleAndStory
    PUBLIC AudioSystem Common EffectsSequences Graphics Input States
           StatesShared VideoSystem
    PRIVATE Assets)
