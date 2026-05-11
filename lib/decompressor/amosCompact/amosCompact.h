#include "unpackedBitmap.h"
#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace decompressor {
namespace amosCompact {

UnpackedBitmap decompress(const std::vector<uint8_t> &compressedData);

}
} // namespace decompressor
} // namespace lib
} // namespace openfranko