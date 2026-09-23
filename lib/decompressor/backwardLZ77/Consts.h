#ifndef BACKWARDLZ77_CONSTS_H_
#define BACKWARDLZ77_CONSTS_H_

#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace decompressor {
namespace backwardLZ77 {
namespace consts {

constexpr size_t FOOTER_SIZE = 20;
constexpr uint32_t BIT_SENTINEL = 0x80000000u;

} // namespace consts
} // namespace backwardLZ77
} // namespace decompressor
} // namespace lib
} // namespace openfranko

#endif // BACKWARDLZ77_CONSTS_H_
