#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace decompressor {
namespace amosCompact {

std::vector<uint8_t> decompress(const std::vector<uint8_t> &compressedData);

}
} // namespace decompressor
} // namespace lib
} // namespace openfranko