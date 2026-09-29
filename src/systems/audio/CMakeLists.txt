add_library(Audio
    Wave.cpp
    Wave.h
    Mixer.cpp
    Mixer.h)
target_include_directories(Audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(Audio PUBLIC ${XMP_TARGET})

add_library(AudioSystem
    AudioSystem.cpp
    AudioSystem.h
    AudioOutput.h
    AudioDevice.cpp
    AudioDevice.h)
target_include_directories(AudioSystem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(AudioSystem PRIVATE ${SDL2_TARGET} PUBLIC Audio)
