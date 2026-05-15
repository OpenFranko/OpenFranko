add_library(TextureManager TextureManager.cpp TextureManager.h)
target_link_libraries(TextureManager PRIVATE SDL2::SDL2 SDL2_image::SDL2_image)