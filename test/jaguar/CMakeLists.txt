add_executable(jaguarSelfTest
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
target_link_options(jaguarProfileGame PRIVATE
    -Wl,--wrap=memcpy -Wl,--wrap=memmove -Wl,--wrap=memset)
target_link_libraries(jaguarProfileGame PRIVATE Engine JaguarHardware)
set(JAGUAR_PROFILE_ONLY "" CACHE STRING "Profile only the named scenario")
if(JAGUAR_PROFILE_ONLY)
    target_compile_definitions(jaguarProfileGame PRIVATE PROFILE_ONLY="${JAGUAR_PROFILE_ONLY}")
endif()
set(JAGUAR_PROFILE_DEFINITIONS "" CACHE STRING "Extra definitions for the profiler")
if(JAGUAR_PROFILE_DEFINITIONS)
    target_compile_definitions(jaguarProfileGame PRIVATE ${JAGUAR_PROFILE_DEFINITIONS})
endif()
set_target_properties(jaguarProfileGame PROPERTIES SUFFIX .elf)
jaguar_binary(jaguarProfileGame)
