add_library(Input
    ControllerSystem.cpp
    ControllerSystem.h)
target_include_directories(Input PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

if(JAGUAR)
    add_library(Platform
        Platform.h
        jaguar/JaguarPlatform.cpp)
    target_link_libraries(Platform PUBLIC Input PRIVATE JaguarHardware)
elseif(DJGPP)
    add_library(Platform
        Platform.h
        allegro/AllegroPlatform.cpp)
    target_link_libraries(Platform PUBLIC Input PRIVATE ${ALLEGRO_TARGET})
else()
    add_library(Platform
        Platform.h
        sdl/SdlPlatform.cpp)
    target_link_libraries(Platform PUBLIC Input PRIVATE ${SDL2_TARGET})
endif()
