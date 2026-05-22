add_library(VideoSystem VideoSystem.cpp VideoSystem.h)
target_link_libraries(VideoSystem PRIVATE SDL2::SDL2 SDL2_image::SDL2_image)

add_library(AudioSystem AudioSystem.cpp AudioSystem.h)
target_link_libraries(AudioSystem PRIVATE SDL2::SDL2 SDL2_mixer::SDL2_mixer)