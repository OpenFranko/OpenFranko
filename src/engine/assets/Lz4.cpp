#include "Lz4.h"

#include <cstring>
#include <stdexcept>

namespace openfranko::src::engine::assets {
namespace {

constexpr unsigned RUN_MASK = 15;
constexpr unsigned EXTENSION_LIMIT = 255;
constexpr std::size_t MIN_MATCH = 4;
constexpr std::size_t SHORT_COPY = 16;

[[noreturn]] void fail() { throw std::runtime_error("Corrupt LZ4 data"); }

std::size_t extendedLength(std::size_t length, const uint8_t *&in,
                           const uint8_t *end) {
  if (length != RUN_MASK) {
    return length;
  }
  unsigned byte = EXTENSION_LIMIT;
  while (byte == EXTENSION_LIMIT) {
    if (in >= end) {
      fail();
    }
    byte = *in++;
    length += byte;
  }
  return length;
}

void copyForward(uint8_t *out, const uint8_t *in, std::size_t length) {
  if (length >= SHORT_COPY) {
    std::memcpy(out, in, length);
    return;
  }
  while (length-- != 0) {
    *out++ = *in++;
  }
}

} // namespace

void decompressLz4(const uint8_t *source, std::size_t sourceSize,
                   uint8_t *target, std::size_t targetSize) {
  const uint8_t *in = source;
  const uint8_t *const inEnd = source + sourceSize;
  uint8_t *out = target;
  uint8_t *const outEnd = target + targetSize;
  while (in < inEnd) {
    const unsigned token = *in++;
    const std::size_t literals = extendedLength(token >> 4, in, inEnd);
    if (literals > static_cast<std::size_t>(inEnd - in) ||
        literals > static_cast<std::size_t>(outEnd - out)) {
      fail();
    }
    copyForward(out, in, literals);
    out += literals;
    in += literals;
    if (in == inEnd) {
      break;
    }
    if (inEnd - in < 2) {
      fail();
    }
    const std::size_t offset =
        static_cast<std::size_t>(in[0]) | static_cast<std::size_t>(in[1]) << 8;
    in += 2;
    if (offset == 0 || offset > static_cast<std::size_t>(out - target)) {
      fail();
    }
    const std::size_t length =
        extendedLength(token & RUN_MASK, in, inEnd) + MIN_MATCH;
    if (length > static_cast<std::size_t>(outEnd - out)) {
      fail();
    }
    const uint8_t *match = out - offset;
    if (offset >= length) {
      copyForward(out, match, length);
      out += length;
    } else if (offset == 1) {
      std::memset(out, *match, length);
      out += length;
    } else {
      for (std::size_t i = 0; i < length; ++i) {
        *out++ = *match++;
      }
    }
  }
  if (out != outEnd) {
    fail();
  }
}

} // namespace openfranko::src::engine::assets
