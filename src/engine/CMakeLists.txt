add_library(Engine Engine.cpp Engine.h)
target_link_libraries(Engine PRIVATE VideoSystem AudioSystem ControllerSystem Player)