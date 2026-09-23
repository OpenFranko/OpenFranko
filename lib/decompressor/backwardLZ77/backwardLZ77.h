#ifndef BACKWARDLZ77_H_
#define BACKWARDLZ77_H_

#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace decompressor {
namespace backwardLZ77 {

std::vector<uint8_t> decompress(const std::vector<uint8_t> &compressedData);

}
} // namespace decompressor
} // namespace lib
} // namespace openfranko

#endif // BACKWARDLZ77_H_
