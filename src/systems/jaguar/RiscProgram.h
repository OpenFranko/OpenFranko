#ifndef SYSTEMS_JAGUAR_RISCPROGRAM_H_
#define SYSTEMS_JAGUAR_RISCPROGRAM_H_

#include <cstddef>
#include <cstdint>

namespace openfranko {
namespace src {
namespace systems {
namespace jaguar {

inline constexpr std::size_t GPU_PROGRAM_HEADER = 0x50;
inline constexpr std::size_t DSP_PROGRAM_HEADER = 0x60;
inline constexpr int HEADER_ENTRIES = 4;

struct RiscProgram {
  const uint8_t *code = nullptr;
  std::size_t size = 0;
  uint32_t ram = 0;
  uint32_t entries[HEADER_ENTRIES] = {};
};

RiscProgram gpuProgram();
RiscProgram dspProgram();
void loadProgram(const RiscProgram &program);

} // namespace jaguar
} // namespace systems
} // namespace src
} // namespace openfranko

#endif // SYSTEMS_JAGUAR_RISCPROGRAM_H_
