add_executable(startAtEnding
    startAtEnding.cpp)
target_link_libraries(startAtEnding
    PRIVATE Common Engine Graphics StreetScenes StreetUi)

add_executable(startAtGameOver
    startAtGameOver.cpp)
target_link_libraries(startAtGameOver PRIVATE Engine StreetUi)

add_executable(startAtLevel1Car
    startAtLevel1Car.cpp)
target_link_libraries(startAtLevel1Car PRIVATE Engine)
