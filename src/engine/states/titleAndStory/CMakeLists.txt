add_library(StatesTitleAndStory
    TitleAndStoryState.cpp
    TitleAndStoryState.h)
target_link_libraries(StatesTitleAndStory
    PUBLIC Assets Audio Common EffectsSequences Graphics Input States
           StatesShared)
