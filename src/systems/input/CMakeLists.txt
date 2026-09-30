add_library(Input
    ControllerSystem.cpp
    ControllerSystem.h)
target_include_directories(Input PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

if(DJGPP)
    add_library(Platform
        AllegroPlatform.cpp
        Platform.h)
    target_link_libraries(Platform PUBLIC Input PRIVATE ${ALLEGRO_TARGET})
else()
    add_library(Platform
        Platform.h
        SdlPlatform.cpp)
    target_link_libraries(Platform PUBLIC Input PRIVATE ${SDL2_TARGET})
endif()
