add_library(Assets
    Assets.cpp
    Assets.h
    Files.h
    GameFiles.h
    Lz4.cpp
    Lz4.h
    PackedArchive.h
    PackedFiles.cpp
    PackedFiles.h
    RequiredFiles.cpp
    RequiredFiles.h
    YieldingFiles.cpp
    YieldingFiles.h)
target_link_libraries(Assets PUBLIC Common Graphics)

if(JAGUAR)
    target_sources(Assets PRIVATE JaguarGameFiles.cpp)
    target_link_libraries(Assets PRIVATE JaguarHardware)
else()
    target_sources(Assets PRIVATE
        ArchiveFiles.cpp
        ArchiveFiles.h
        DiskFiles.cpp
        DiskFiles.h
        GameFiles.cpp)
endif()
