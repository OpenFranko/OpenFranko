add_library(Collision Collision.cpp Collision.h)
target_link_libraries(Collision PRIVATE SDL2::SDL2 SDL2_image::SDL2_image)