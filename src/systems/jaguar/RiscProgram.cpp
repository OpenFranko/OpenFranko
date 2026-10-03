#include "RiscProgram.h"

#include "Hardware.h"

extern "C" {
extern const uint8_t jaguarGpuProgram[];
extern const uint8_t jaguarGpuProgramEnd[];
extern const uint8_t jaguarDspProgram[];
extern const uint8_t jaguarDspProgramEnd[];
}

namespace openfranko::src::systems::jaguar {
namespace {

uint32_t readLong(const uint8_t *at) {
  return static_cast<uint32_t>(at[0]) << 24 |
         static_cast<uint32_t>(at[1]) << 16 |
         static_cast<uint32_t>(at[2]) << 8 | static_cast<uint32_t>(at[3]);
}

RiscProgram describe(const uint8_t *begin, const uint8_t *end, uint32_t ram,
                     std::size_t header) {
  RiscProgram program;
  program.code = begin;
  program.size = static_cast<std::size_t>(end - begin);
  program.ram = ram;
  for (int entry = 0; entry < HEADER_ENTRIES; ++entry) {
    program.entries[entry] = readLong(begin + header + 4 * entry);
  }
  return program;
}

} // namespace

RiscProgram gpuProgram() {
  return describe(jaguarGpuProgram, jaguarGpuProgramEnd, GPU_RAM,
                  GPU_PROGRAM_HEADER);
}

RiscProgram dspProgram() {
  return describe(jaguarDspProgram, jaguarDspProgramEnd, DSP_RAM,
                  DSP_PROGRAM_HEADER);
}

void loadProgram(const RiscProgram &program) {
  for (std::size_t at = 0; at < program.size; at += 4) {
    uint32_t value = 0;
    for (std::size_t byte = 0; byte < 4; ++byte) {
      value =
          value << 8 | (at + byte < program.size ? program.code[at + byte] : 0);
    }
    longWord(program.ram + static_cast<uint32_t>(at)) = value;
  }
}

} // namespace openfranko::src::systems::jaguar
