#ifndef LZ4COMPRESSOR_H_
#define LZ4COMPRESSOR_H_

#include <cstddef>
#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace converter {
namespace packedArchive {

std::vector<uint8_t> compressLz4(const uint8_t *data, std::size_t size);

} // namespace packedArchive
} // namespace converter
} // namespace lib
} // namespace openfranko

#endif // LZ4COMPRESSOR_H_
