add_subdirectory(engine)
add_subdirectory(systems)

add_executable(${PROJECT_NAME}
    main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE Engine)
