add_library(Game Game.cpp Game.h)
target_link_libraries(Game PRIVATE SDL2::SDL2 SDL2_mixer::SDL2_mixer SDL2_image::SDL2_image)