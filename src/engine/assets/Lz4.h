#ifndef ENGINE_ASSETS_LZ4_H_
#define ENGINE_ASSETS_LZ4_H_

#include <cstddef>
#include <cstdint>

namespace openfranko {
namespace src {
namespace engine {
namespace assets {

void decompressLz4(const uint8_t *source, std::size_t sourceSize,
                   uint8_t *target, std::size_t targetSize);

} // namespace assets
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_ASSETS_LZ4_H_
