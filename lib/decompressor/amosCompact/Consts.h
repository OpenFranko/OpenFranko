#ifndef CONSTS_H_
#define CONSTS_H_

#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace decompressor {
namespace amosCompact {
namespace consts {

constexpr uint32_t MINIMAL_SIZE = 4;

constexpr uint32_t SPACK_SCREEN_HEADER = 0x12031990u;
constexpr uint32_t AMOS_BMCODE = 0x06071963u;

constexpr size_t MAX_SUPPORTED_BITPLANES = 6;

constexpr size_t SPACK_HEADER_SIZE = 90;

constexpr size_t SPACK_PALETTE_SIZE = 32;

constexpr size_t PACKED_BITMAP_HEADER_SIZE = 24;

} // namespace consts
} // namespace amosCompact
} // namespace decompressor
} // namespace lib
} // namespace openfranko

#endif // CONSTS_H_