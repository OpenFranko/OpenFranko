add_library(Map Map.cpp Map.h)
target_link_libraries(Map PRIVATE TextureManager SDL2::SDL2 SDL2_image::SDL2_image)