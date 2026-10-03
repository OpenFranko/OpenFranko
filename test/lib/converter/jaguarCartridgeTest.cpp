#include "../../../lib/converter/jaguarCartridge/jaguarCartridge.h"

#include <catch2/catch_all.hpp>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

using namespace openfranko::lib::converter::jaguarCartridge;

namespace {

constexpr std::size_t CODE_OFFSET = CODE_BASE - CART_BASE;
constexpr std::size_t HEADER_SIZE = 12;

std::vector<uint8_t> counting(std::size_t size, uint8_t first) {
  std::vector<uint8_t> bytes(size);
  for (std::size_t at = 0; at < size; ++at) {
    bytes[at] = static_cast<uint8_t>(first + at);
  }
  return bytes;
}

bool all(const std::vector<uint8_t> &image, std::size_t first, std::size_t last,
         uint8_t value) {
  for (std::size_t at = first; at < last; ++at) {
    if (image[at] != value) {
      return false;
    }
  }
  return true;
}

std::vector<uint8_t> slice(const std::vector<uint8_t> &image, std::size_t first,
                           std::size_t size) {
  return std::vector<uint8_t>(
      image.begin() + static_cast<std::ptrdiff_t>(first),
      image.begin() + static_cast<std::ptrdiff_t>(first + size));
}

} // namespace

SCENARIO("The payload is the program followed by the asset archive") {
  GIVEN("A program and an archive") {
    const std::vector<uint8_t> program{1, 2, 3, 4};
    const std::vector<uint8_t> archive{9, 8};

    THEN("The archive starts where the program ends") {
      REQUIRE(payload(program, archive) ==
              std::vector<uint8_t>{1, 2, 3, 4, 9, 8});
    }
  }
}

SCENARIO("An unsigned cartridge has the boot header and the payload at "
         "0x802000") {
  GIVEN("A small payload") {
    const std::vector<uint8_t> bytes = counting(40, 7);
    const std::vector<uint8_t> image = unsignedImage(bytes);

    THEN("It fills a 4 MB cartridge") { REQUIRE(image.size() == CART_SIZE); }

    THEN("The first 1 KB is zero and the header follows it") {
      REQUIRE(all(image, 0, HEADER_OFFSET, 0x00));
      REQUIRE(slice(image, HEADER_OFFSET, HEADER_SIZE) ==
              std::vector<uint8_t>{0x04, 0x04, 0x04, 0x04, 0x00, 0x80, 0x20,
                                   0x00, 0x00, 0x00, 0x00, 0x00});
    }

    THEN("Erased flash fills the rest of the header area and the space after "
         "the payload") {
      REQUIRE(all(image, HEADER_OFFSET + HEADER_SIZE, CODE_OFFSET, 0xFF));
      REQUIRE(all(image, CODE_OFFSET + bytes.size(), image.size(), 0xFF));
    }

    THEN("The payload starts at the code address") {
      REQUIRE(slice(image, CODE_OFFSET, bytes.size()) == bytes);
    }
  }
}

SCENARIO("The cartridge grows from 4 MB to 6 MB and no further") {
  GIVEN("Payloads around the limits") {
    THEN("A payload that just fits 4 MB keeps 4 MB") {
      REQUIRE(cartridgeSize(CART_SIZE - CODE_OFFSET) == CART_SIZE);
    }

    THEN("One byte more needs 6 MB") {
      REQUIRE(cartridgeSize(CART_SIZE - CODE_OFFSET + 1) == MAX_CART_SIZE);
    }

    THEN("A payload that just fits 6 MB is accepted") {
      REQUIRE(cartridgeSize(MAX_CART_SIZE - CODE_OFFSET) == MAX_CART_SIZE);
    }

    THEN("Anything larger is refused") {
      REQUIRE_THROWS_AS(cartridgeSize(MAX_CART_SIZE - CODE_OFFSET + 1),
                        std::runtime_error);
    }
  }

  GIVEN("A payload past 4 MB") {
    const std::vector<uint8_t> bytes(CART_SIZE - CODE_OFFSET + 8, 0x5A);
    const std::vector<uint8_t> image = unsignedImage(bytes);

    THEN("The image is 6 MB with erased flash after the payload") {
      REQUIRE(image.size() == MAX_CART_SIZE);
      REQUIRE(all(image, CODE_OFFSET, CODE_OFFSET + bytes.size(), 0x5A));
      REQUIRE(all(image, CODE_OFFSET + bytes.size(), image.size(), 0xFF));
    }
  }
}
