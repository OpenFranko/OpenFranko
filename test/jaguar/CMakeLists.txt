add_executable(jaguarSelfTest
    ../../lib/converter/packedArchive/lz4Compressor.cpp
    ../../src/systems/graphics/jaguar/JaguarPixelOps.cpp
    selfTest.cpp)
target_link_libraries(jaguarSelfTest PRIVATE JaguarHardware)
set_target_properties(jaguarSelfTest PROPERTIES SUFFIX .elf)
jaguar_binary(jaguarSelfTest)

add_executable(jaguarProfileGame
    memoryCalls.cpp
    memoryCalls.h
    profileGame.cpp
    sampler.cpp
    sampler.h)
target_link_options(jaguarProfileGame PRIVATE -Wl,--wrap=getentropy)
target_link_libraries(jaguarProfileGame PRIVATE Engine JaguarHardware)
set(JAGUAR_PROFILE_ONLY "" CACHE STRING "Profile only the named scenario")
if(JAGUAR_PROFILE_ONLY)
    target_compile_definitions(jaguarProfileGame PRIVATE PROFILE_ONLY="${JAGUAR_PROFILE_ONLY}")
endif()
set(JAGUAR_PROFILE_DEFINITIONS "" CACHE STRING "Extra definitions for the profiler")
if(JAGUAR_PROFILE_DEFINITIONS)
    target_compile_definitions(jaguarProfileGame PRIVATE ${JAGUAR_PROFILE_DEFINITIONS})
endif()
if("PROFILE_MEMORY" IN_LIST JAGUAR_PROFILE_DEFINITIONS)
    target_link_options(jaguarProfileGame PRIVATE
        -Wl,--wrap=memcpy -Wl,--wrap=memmove -Wl,--wrap=memset)
endif()
if("PROFILE_ARITHMETIC" IN_LIST JAGUAR_PROFILE_DEFINITIONS)
    target_link_options(jaguarProfileGame PRIVATE
        -Wl,--wrap=__mulsi3 -Wl,--wrap=__udivsi3 -Wl,--wrap=__divsi3
        -Wl,--wrap=__umodsi3 -Wl,--wrap=__modsi3 -Wl,--wrap=__muldi3)
endif()
set_target_properties(jaguarProfileGame PROPERTIES SUFFIX .elf)
jaguar_binary(jaguarProfileGame)
