add_subdirectory(entityComponentSystem)
add_subdirectory(collision)
add_subdirectory(vector2d)
add_subdirectory(textureManager)
add_subdirectory(game)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} Game)