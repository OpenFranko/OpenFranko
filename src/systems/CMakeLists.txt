add_library(Graphics
    graphics/Bitmap.cpp
    graphics/Bitmap.h
    graphics/Canvas.cpp
    graphics/Canvas.h
    graphics/Display.cpp
    graphics/Display.h)
target_include_directories(Graphics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(VideoSystem
    graphics/VideoSystem.cpp
    graphics/VideoSystem.h)
target_link_libraries(VideoSystem PRIVATE ${SDL2_TARGET} PUBLIC Graphics)
target_include_directories(VideoSystem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(Audio
    audio/Wave.cpp
    audio/Wave.h
    audio/Mixer.cpp
    audio/Mixer.h)
target_include_directories(Audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Audio PUBLIC ${XMP_TARGET})

add_library(AudioSystem
    audio/AudioSystem.cpp
    audio/AudioSystem.h
    audio/AudioOutput.h
    audio/AudioDevice.cpp
    audio/AudioDevice.h)
target_include_directories(AudioSystem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(AudioSystem PRIVATE ${SDL2_TARGET} PUBLIC Audio)

add_library(Input
    input/ControllerSystem.cpp
    input/ControllerSystem.h)
target_include_directories(Input PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(Platform
    input/Platform.cpp
    input/Platform.h)
target_include_directories(Platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Platform PRIVATE ${SDL2_TARGET} PUBLIC Input)