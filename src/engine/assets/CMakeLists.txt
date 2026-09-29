add_library(Assets
    Assets.cpp
    Assets.h
    DiskFiles.cpp
    DiskFiles.h
    Files.h)
target_link_libraries(Assets PUBLIC Common Graphics)
