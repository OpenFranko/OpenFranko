execute_process(
    COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=crtbegin.o
    OUTPUT_VARIABLE JAGUAR_CRTBEGIN OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(
    COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=crtend.o
    OUTPUT_VARIABLE JAGUAR_CRTEND OUTPUT_STRIP_TRAILING_WHITESPACE)

find_program(JAGUAR_RMAC rmac
    HINTS ENV JAGSDK
    PATH_SUFFIXES tools/bin
    REQUIRED)

foreach(program Gpu Dsp)
    string(TOLOWER ${program} name)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${name}.bin
        COMMAND ${JAGUAR_RMAC} -fr -o ${CMAKE_CURRENT_BINARY_DIR}/${name}.bin
                ${CMAKE_CURRENT_SOURCE_DIR}/risc/${program}.s
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/risc/${program}.s
        VERBATIM)
endforeach()

set_source_files_properties(RiscBlobs.S PROPERTIES
    COMPILE_OPTIONS "-Wa,-I${CMAKE_CURRENT_BINARY_DIR}"
    OBJECT_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/gpu.bin;${CMAKE_CURRENT_BINARY_DIR}/dsp.bin")

add_library(JaguarHardware
    Blitter.cpp
    Blitter.h
    Console.cpp
    Console.h
    ConsoleFont.cpp
    ConsoleFont.h
    crt0.S
    DebugOverlay.cpp
    DebugOverlay.h
    Eeprom.cpp
    Eeprom.h
    FrameBuilder.cpp
    FrameBuilder.h
    Hardware.h
    Interrupts.S
    Joypad.cpp
    Joypad.h
    Memory.S
    Memory.cpp
    ObjectList.cpp
    ObjectList.h
    Profiler.cpp
    Profiler.h
    TextPanel.cpp
    TextPanel.h
    VirtualKeyboard.cpp
    VirtualKeyboard.h
    RiscBlobs.S
    RiscProgram.cpp
    RiscProgram.h
    Runtime.cpp
    Runtime.h
    Video.cpp
    Video.h)
target_include_directories(JaguarHardware PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
set_source_files_properties(Memory.cpp PROPERTIES
    COMPILE_OPTIONS "-fno-builtin;-fno-tree-loop-distribute-patterns;-fno-lto")

option(JAGUAR_DEBUG_OVERLAY "Show frame timing and memory use on the Jaguar" OFF)
if(JAGUAR_DEBUG_OVERLAY)
    target_compile_definitions(JaguarHardware PUBLIC JAGUAR_DEBUG_OVERLAY)
endif()
target_link_options(JaguarHardware INTERFACE
    -T ${CMAKE_CURRENT_SOURCE_DIR}/jaguar.ld
    -Wl,-u,_start
    -Wl,-u,memcpy
    -Wl,-u,memmove
    -Wl,-u,memset
    -Wl,-u,memcmp
    -Wl,--no-warn-rwx-segments
    ${JAGUAR_CRTBEGIN}
    ${JAGUAR_CRTEND})
target_link_libraries(JaguarHardware INTERFACE stdc++ c gcc)

function(jaguar_binary target)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${target}>
                $<TARGET_FILE_DIR:${target}>/$<TARGET_FILE_BASE_NAME:${target}>.bin
        VERBATIM)
endfunction()
