#include "headers.h"
#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace decompressor {
namespace amosCompact {

std::vector<uint8_t> bitmapUnpack(const std::vector<uint8_t> &packedData,
                                  const headers::BitmapHeader &header,
                                  const std::vector<uint16_t> &palette);

}
} // namespace decompressor
} // namespace lib
} // namespace openfranko