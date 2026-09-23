#ifndef CODECARDS_H_
#define CODECARDS_H_

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace openfranko {
namespace lib {
namespace converter {
namespace codeCards {

namespace consts {

inline constexpr size_t CARD_COUNT = 2;
inline constexpr size_t CARD_SIZE = 10;
inline constexpr size_t FIRST_CARD_OFFSET = 10;
inline constexpr uint8_t COLOR_COUNT = 11;

} // namespace consts

namespace colors {

inline constexpr const char *name(uint8_t color) {
  switch (color) {
  case 0:
    return "black";
  case 1:
    return "white";
  case 2:
    return "yellow";
  case 3:
    return "orange";
  case 4:
    return "red";
  case 5:
    return "brown";
  case 6:
    return "pink";
  case 7:
    return "light blue";
  case 8:
    return "dark blue";
  case 9:
    return "light green";
  case 10:
    return "dark green";
  default:
    return "unknown";
  }
}

} // namespace colors

struct Card {
  std::array<std::array<uint8_t, consts::CARD_SIZE>, consts::CARD_SIZE> rows{};
};

using CodeCards = std::array<Card, consts::CARD_COUNT>;

CodeCards parse(const std::vector<uint8_t> &decompressedData);

std::vector<uint8_t> toJson(const CodeCards &cards);

} // namespace codeCards
} // namespace converter
} // namespace lib
} // namespace openfranko

#endif // CODECARDS_H_
