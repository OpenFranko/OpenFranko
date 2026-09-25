add_library(Graphics Bitmap.cpp Bitmap.h Canvas.cpp Canvas.h Display.cpp Display.h)

add_library(VideoSystem VideoSystem.cpp VideoSystem.h)
target_link_libraries(VideoSystem PRIVATE ${SDL2_TARGET} PUBLIC Graphics)

add_library(Mixer Wave.cpp Wave.h Mixer.cpp Mixer.h)
target_link_libraries(Mixer PUBLIC ${XMP_TARGET})

add_library(AudioSystem AudioSystem.cpp AudioSystem.h AudioOutput.h AudioDevice.cpp AudioDevice.h)
target_link_libraries(AudioSystem PRIVATE ${SDL2_TARGET} PUBLIC Mixer)

add_library(ControllerSystem ControllerSystem.cpp ControllerSystem.h)

add_library(Platform Platform.cpp Platform.h)
target_link_libraries(Platform PRIVATE ${SDL2_TARGET} PUBLIC ControllerSystem)