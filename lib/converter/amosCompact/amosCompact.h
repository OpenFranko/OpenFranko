#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace converter {
namespace amosCompact {

std::vector<uint8_t> decompress(const std::vector<uint8_t> &compressedData);

}
} // namespace converter
} // namespace lib
} // namespace openfranko