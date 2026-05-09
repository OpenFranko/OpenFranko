#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace decompressor {
namespace backwardLZ77 {
namespace helpers {

uint32_t readUint32BigEndian(const std::vector<uint8_t> &data, size_t pos);

} // namespace helpers
} // namespace backwardLZ77
} // namespace decompressor
} // namespace lib
} // namespace openfranko