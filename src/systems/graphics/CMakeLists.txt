add_library(Graphics
    Bitmap.cpp
    Bitmap.h
    Canvas.cpp
    Canvas.h
    Display.cpp
    Display.h
    IndexedRasterizer.cpp
    IndexedRasterizer.h
    Monitor.h
    ScreenMode.cpp
    ScreenMode.h)
target_include_directories(Graphics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

if(DJGPP)
    add_library(VideoSystem
        AllegroVideoSystem.cpp
        VideoSystem.cpp
        VideoSystem.h)
    target_link_libraries(VideoSystem PUBLIC Graphics PRIVATE ${ALLEGRO_TARGET})
else()
    add_library(VideoSystem
        SdlVideoSystem.cpp
        VideoSystem.cpp
        VideoSystem.h)
    target_link_libraries(VideoSystem PUBLIC Graphics PRIVATE ${SDL2_TARGET})
endif()
