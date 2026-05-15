add_library(Game Game.cpp Game.h)
target_link_libraries(Game ${SDL2_LIBRARIES} ${SDL2_IMAGE_LIBRARIES} ${SDL2_MIXER_LIBRARIES})