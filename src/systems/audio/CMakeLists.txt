add_library(Audio
    Mixer.cpp
    Mixer.h
    Speaker.h
    Wave.cpp
    Wave.h)
target_include_directories(Audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(Audio PRIVATE ${XMP_TARGET})

add_library(AudioSystem
    AudioDevice.cpp
    AudioDevice.h
    AudioOutput.h
    AudioSystem.cpp
    AudioSystem.h)
target_link_libraries(AudioSystem PUBLIC Audio PRIVATE ${SDL2_TARGET})
