add_library(Player Player.cpp Player.h)
target_link_libraries(Player PRIVATE VideoSystem ControllerSystem)