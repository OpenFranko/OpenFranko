add_library(GameObject GameObject.cpp GameObject.h)
target_link_libraries(GameObject PRIVATE TextureManager SDL2::SDL2 SDL2_image::SDL2_image)