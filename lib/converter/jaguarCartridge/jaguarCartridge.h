#ifndef JAGUARCARTRIDGE_H_
#define JAGUARCARTRIDGE_H_

#include <cstddef>
#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace converter {
namespace jaguarCartridge {

inline constexpr std::size_t CART_BASE = 0x800000;
inline constexpr std::size_t CODE_BASE = 0x802000;
inline constexpr std::size_t HEADER_OFFSET = 0x400;
inline constexpr std::size_t CART_SIZE = 0x400000;
inline constexpr std::size_t MAX_CART_SIZE = 0x600000;

std::vector<uint8_t> payload(const std::vector<uint8_t> &program,
                             const std::vector<uint8_t> &archive);
std::size_t cartridgeSize(std::size_t payloadSize);
std::vector<uint8_t> unsignedImage(const std::vector<uint8_t> &payload);

} // namespace jaguarCartridge
} // namespace converter
} // namespace lib
} // namespace openfranko

#endif // JAGUARCARTRIDGE_H_
