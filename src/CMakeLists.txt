add_subdirectory(textureManager)
add_subdirectory(gameObject)
add_subdirectory(map)
add_subdirectory(game)

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} Game)