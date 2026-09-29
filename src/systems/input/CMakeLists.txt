add_library(Input
    ControllerSystem.cpp
    ControllerSystem.h)
target_include_directories(Input PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(Platform
    Platform.cpp
    Platform.h)
target_include_directories(Platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(Platform PRIVATE ${SDL2_TARGET} PUBLIC Input)
