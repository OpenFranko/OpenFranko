add_library(Assets
    ArchiveFiles.cpp
    ArchiveFiles.h
    Assets.cpp
    Assets.h
    DiskFiles.cpp
    DiskFiles.h
    Files.h
    RequiredFiles.cpp
    RequiredFiles.h
    YieldingFiles.cpp
    YieldingFiles.h)
target_link_libraries(Assets PUBLIC Common Graphics)
