add_library(Graphics
    Bitmap.cpp
    Bitmap.h
    Canvas.cpp
    Canvas.h
    Display.cpp
    Display.h
    IndexedRasterizer.cpp
    IndexedRasterizer.h
    KeptRuns.h
    Monitor.h
    PixelOps.h
    PlanarFrame.cpp
    PlanarFrame.h
    ScreenMode.cpp
    ScreenMode.h)
target_include_directories(Graphics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

if(JAGUAR)
    target_sources(Graphics PRIVATE jaguar/JaguarPixelOps.cpp)
    target_link_libraries(Graphics PRIVATE JaguarHardware)
else()
    target_sources(Graphics PRIVATE PixelOps.cpp)
endif()

if(JAGUAR)
    add_library(VideoSystem
        VideoSystem.h
        jaguar/JaguarVideoSystem.cpp)
    target_link_libraries(VideoSystem PUBLIC Graphics PRIVATE JaguarHardware)
elseif(DJGPP)
    add_library(VideoSystem
        VideoSystem.cpp
        VideoSystem.h
        allegro/AllegroVideoSystem.cpp)
    target_link_libraries(VideoSystem PUBLIC Graphics PRIVATE ${ALLEGRO_TARGET})
else()
    add_library(VideoSystem
        VideoSystem.cpp
        VideoSystem.h
        sdl/SdlVideoSystem.cpp)
    target_link_libraries(VideoSystem PUBLIC Graphics PRIVATE ${SDL2_TARGET})
endif()
