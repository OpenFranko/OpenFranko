add_library(Graphics
    Bitmap.cpp
    Bitmap.h
    Canvas.cpp
    Canvas.h
    Display.cpp
    Display.h)
target_include_directories(Graphics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(VideoSystem
    VideoSystem.cpp
    VideoSystem.h)
target_link_libraries(VideoSystem PRIVATE ${SDL2_TARGET} PUBLIC Graphics)
target_include_directories(VideoSystem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
