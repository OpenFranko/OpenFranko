add_library(Engine Engine.cpp Engine.h)
target_link_libraries(Engine PRIVATE SDL2::SDL2 SDL2_image::SDL2_image SDL2_mixer::SDL2_mixer)