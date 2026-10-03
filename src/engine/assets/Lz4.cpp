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

void decodeSequences(const uint8_t *&in, const uint8_t *inEnd,
                     const uint8_t *target, uint8_t *&out,
                     const uint8_t *outEnd, const uint8_t *limit) {
  while (in < inEnd && (limit == nullptr || out < limit)) {
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
}

} // namespace

void decompressLz4(const uint8_t *source, std::size_t sourceSize,
                   uint8_t *target, std::size_t targetSize) {
  const uint8_t *in = source;
  uint8_t *out = target;
  decodeSequences(in, source + sourceSize, target, out, target + targetSize,
                  nullptr);
  if (out != target + targetSize) {
    fail();
  }
}

void decompressLz4Part(const uint8_t *&source, const uint8_t *sourceEnd,
                       const uint8_t *start, uint8_t *&target,
                       const uint8_t *targetEnd, const uint8_t *limit) {
  decodeSequences(source, sourceEnd, start, target, targetEnd, limit);
}

Lz4Steps::Lz4Steps(const uint8_t *source, std::size_t sourceSize,
                   uint8_t *target, std::size_t targetSize)
    : m_source(source), m_sourceEnd(source + sourceSize), m_start(target),
      m_target(target), m_targetEnd(target + targetSize) {}

bool Lz4Steps::step(std::size_t bytes) {
  const std::size_t room = static_cast<std::size_t>(m_targetEnd - m_target);
  const uint8_t *limit = bytes < room ? m_target + bytes : nullptr;
  unpackLz4Part(m_source, m_sourceEnd, m_start, m_target, m_targetEnd, limit);
  if (m_source == m_sourceEnd) {
    if (m_target != m_targetEnd) {
      fail();
    }
    return true;
  }
  if (!limit || m_target < limit) {
    fail();
  }
  return false;
}

} // namespace openfranko::src::engine::assets
