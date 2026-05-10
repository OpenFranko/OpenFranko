#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace decompressor {
namespace helpers {

uint32_t readUint32BigEndian(const std::vector<uint8_t> &data, size_t pos);

uint16_t readUint16BigEndian(const std::vector<uint8_t> &data, size_t pos);

int16_t readInt16BigEndian(const std::vector<uint8_t> &data, size_t pos);

} // namespace helpers
} // namespace decompressor
} // namespace lib
} // namespace openfranko