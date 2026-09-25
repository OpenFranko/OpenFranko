add_library(Graphics Bitmap.cpp Bitmap.h Canvas.cpp Canvas.h)

add_library(VideoSystem VideoSystem.cpp VideoSystem.h)
target_link_libraries(VideoSystem PRIVATE SDL2::SDL2)

add_library(Mixer Wave.cpp Wave.h Mixer.cpp Mixer.h)
target_link_libraries(Mixer PUBLIC PkgConfig::XMP)

add_library(AudioSystem AudioSystem.cpp AudioSystem.h AudioDevice.cpp AudioDevice.h)
target_link_libraries(AudioSystem PRIVATE SDL2::SDL2 PUBLIC Mixer)

add_library(ControllerSystem ControllerSystem.cpp ControllerSystem.h)

add_library(Platform Platform.cpp Platform.h)
target_link_libraries(Platform PRIVATE SDL2::SDL2 PUBLIC ControllerSystem)