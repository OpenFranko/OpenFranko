#include "jaguarCartridge.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>

namespace openfranko::lib::converter::jaguarCartridge {
namespace {

constexpr uint8_t ERASED = 0xFF;
constexpr std::array<uint8_t, 12> HEADER = {0x04, 0x04, 0x04, 0x04, 0x00, 0x80,
                                            0x20, 0x00, 0x00, 0x00, 0x00, 0x00};

} // namespace

std::vector<uint8_t> payload(const std::vector<uint8_t> &program,
                             const std::vector<uint8_t> &archive) {
  std::vector<uint8_t> result;
  result.reserve(program.size() + archive.size());
  result.insert(result.end(), program.begin(), program.end());
  result.insert(result.end(), archive.begin(), archive.end());
  return result;
}

std::size_t cartridgeSize(std::size_t payloadSize) {
  const std::size_t total = CODE_BASE - CART_BASE + payloadSize;
  if (total > MAX_CART_SIZE) {
    throw std::runtime_error(
        "The cartridge needs " + std::to_string(total) +
        " bytes, more than the 6 MB a Jaguar cartridge holds");
  }
  return total > CART_SIZE ? MAX_CART_SIZE : CART_SIZE;
}

std::vector<uint8_t> unsignedImage(const std::vector<uint8_t> &payload) {
  std::vector<uint8_t> image(cartridgeSize(payload.size()), ERASED);
  std::fill(image.begin(), image.begin() + HEADER_OFFSET, 0);
  std::copy(HEADER.begin(), HEADER.end(), image.begin() + HEADER_OFFSET);
  std::copy(payload.begin(), payload.end(),
            image.begin() + (CODE_BASE - CART_BASE));
  return image;
}

} // namespace openfranko::lib::converter::jaguarCartridge
