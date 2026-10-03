#ifndef ENGINE_ASSETS_PACKEDARCHIVE_H_
#define ENGINE_ASSETS_PACKEDARCHIVE_H_

#include <cstddef>
#include <cstdint>

namespace openfranko {
namespace src {
namespace engine {
namespace assets {
namespace packed {

inline constexpr uint32_t MAGIC = 0x4F465041;
inline constexpr uint32_t VERSION = 1;
inline constexpr std::size_t HEADER_SIZE = 16;
inline constexpr std::size_t ENTRY_SIZE = 20;
inline constexpr std::size_t MAGIC_OFFSET = 0;
inline constexpr std::size_t VERSION_OFFSET = 4;
inline constexpr std::size_t COUNT_OFFSET = 8;
inline constexpr std::size_t SIZE_OFFSET = 12;
inline constexpr std::size_t NAME_OFFSET = 0;
inline constexpr std::size_t DATA_OFFSET = 4;
inline constexpr std::size_t STORED_SIZE_OFFSET = 8;
inline constexpr std::size_t UNPACKED_SIZE_OFFSET = 12;
inline constexpr std::size_t FLAGS_OFFSET = 16;
inline constexpr uint32_t COMPRESSED = 1;
inline constexpr uint32_t BITMAP = 2;
inline constexpr std::size_t BITMAP_HEADER_SIZE = 12;
inline constexpr std::size_t BITMAP_WIDTH_OFFSET = 0;
inline constexpr std::size_t BITMAP_HEIGHT_OFFSET = 2;
inline constexpr std::size_t BITMAP_HOTSPOT_X_OFFSET = 4;
inline constexpr std::size_t BITMAP_HOTSPOT_Y_OFFSET = 6;
inline constexpr std::size_t BITMAP_COLORS_OFFSET = 8;
inline constexpr std::size_t DATA_ALIGNMENT = 8;

inline uint32_t readLong(const uint8_t *at) {
  return static_cast<uint32_t>(at[0]) << 24 |
         static_cast<uint32_t>(at[1]) << 16 |
         static_cast<uint32_t>(at[2]) << 8 | static_cast<uint32_t>(at[3]);
}

inline uint16_t readWord(const uint8_t *at) {
  return static_cast<uint16_t>(at[0] << 8 | at[1]);
}

} // namespace packed
} // namespace assets
} // namespace engine
} // namespace src
} // namespace openfranko

#endif // ENGINE_ASSETS_PACKEDARCHIVE_H_
