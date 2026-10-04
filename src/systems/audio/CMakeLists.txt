if(JAGUAR)
    add_library(Audio
        Mixer.h
        S3m.cpp
        S3m.h
        Speaker.h
        Tracker.cpp
        Tracker.h
        Wave.cpp
        Wave.h)
    target_include_directories(Audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
    add_library(AudioSystem
        AudioSystem.h
        jaguar/JaguarAudioSystem.cpp)
    target_link_libraries(AudioSystem PUBLIC Audio PRIVATE JaguarHardware)
    return()
endif()

add_library(Audio
    Mixer.cpp
    Mixer.h
    S3m.cpp
    S3m.h
    Speaker.h
    Tracker.cpp
    Tracker.h
    Wave.cpp
    Wave.h)
target_include_directories(Audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(Audio PRIVATE ${XMP_TARGET})

if(DJGPP)
    add_library(AudioSystem
        AudioDevice.h
        AudioOutput.h
        AudioSystem.cpp
        AudioSystem.h
        allegro/AllegroAudioDevice.cpp)
    target_link_libraries(AudioSystem PUBLIC Audio PRIVATE ${ALLEGRO_TARGET})
else()
    add_library(AudioSystem
        AudioDevice.h
        AudioOutput.h
        AudioSystem.cpp
        AudioSystem.h
        sdl/SdlAudioDevice.cpp)
    target_link_libraries(AudioSystem PUBLIC Audio PRIVATE ${SDL2_TARGET})
endif()
