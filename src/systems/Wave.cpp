#include "Wave.h"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iterator>
#include <optional>
#include <stdexcept>

namespace openfranko::src::systems {
namespace {

constexpr std::size_t RIFF_HEADER_SIZE = 12;
constexpr std::size_t CHUNK_HEADER_SIZE = 8;
constexpr std::size_t FORMAT_SIZE = 16;
constexpr uint16_t PCM = 1;
constexpr int BYTE_OFFSET = 128;
constexpr int BYTE_SHIFT = 8;

struct Format {
  uint16_t tag;
  uint16_t channels;
  uint32_t rate;
  uint16_t bits;
};

[[noreturn]] void fail(const std::string &cause) {
  throw std::runtime_error(cause);
}

uint32_t readLittleEndian(const uint8_t *bytes, std::size_t size) {
  uint32_t value = 0;
  for (std::size_t i = size; i > 0; --i) {
    value = value << 8 | bytes[i - 1];
  }
  return value;
}

bool isChunk(const uint8_t *bytes, const char *id) {
  return std::memcmp(bytes, id, 4) == 0;
}

int sampleAt(const uint8_t *bytes, uint16_t bits) {
  if (bits == 8) {
    return (bytes[0] - BYTE_OFFSET) << BYTE_SHIFT;
  }
  return static_cast<int16_t>(readLittleEndian(bytes, 2));
}

} // namespace

Sound readWave(const std::vector<uint8_t> &file) {
  if (file.size() < RIFF_HEADER_SIZE || !isChunk(file.data(), "RIFF") ||
      !isChunk(file.data() + 8, "WAVE")) {
    fail("Not a WAVE file");
  }

  std::optional<Format> format;
  const uint8_t *data = nullptr;
  std::size_t dataSize = 0;
  std::size_t offset = RIFF_HEADER_SIZE;
  while (offset + CHUNK_HEADER_SIZE <= file.size()) {
    const uint8_t *chunk = file.data() + offset;
    const std::size_t size = readLittleEndian(chunk + 4, 4);
    const std::size_t body = offset + CHUNK_HEADER_SIZE;
    const std::size_t available = std::min(size, file.size() - body);
    if (isChunk(chunk, "fmt ")) {
      if (available < FORMAT_SIZE) {
        fail("Truncated WAVE format");
      }
      const uint8_t *fields = file.data() + body;
      format = Format{static_cast<uint16_t>(readLittleEndian(fields, 2)),
                      static_cast<uint16_t>(readLittleEndian(fields + 2, 2)),
                      readLittleEndian(fields + 4, 4),
                      static_cast<uint16_t>(readLittleEndian(fields + 14, 2))};
    } else if (isChunk(chunk, "data")) {
      data = file.data() + body;
      dataSize = available;
    }
    offset = body + size + (size & 1);
  }

  if (!format || !data) {
    fail("Missing WAVE chunks");
  }
  if (format->tag != PCM || format->channels < 1 || format->channels > 2 ||
      (format->bits != 8 && format->bits != 16) || format->rate == 0) {
    fail("Unsupported WAVE format");
  }

  const std::size_t sampleBytes = format->bits / 8;
  const std::size_t frameBytes = sampleBytes * format->channels;
  const std::size_t count = dataSize / frameBytes;
  Sound sound;
  sound.rate = static_cast<int>(format->rate);
  sound.frames.reserve(count);
  for (std::size_t frame = 0; frame < count; ++frame) {
    int sum = 0;
    for (std::size_t channel = 0; channel < format->channels; ++channel) {
      sum += sampleAt(data + frame * frameBytes + channel * sampleBytes,
                      format->bits);
    }
    sound.frames.push_back(
        static_cast<int8_t>((sum / format->channels) >> BYTE_SHIFT));
  }
  return sound;
}

Sound loadWave(const std::string &path) {
  std::ifstream stream(path, std::ios::binary);
  if (!stream) {
    throw std::runtime_error("Failed to load sound: " + path);
  }
  const std::vector<uint8_t> file{std::istreambuf_iterator<char>(stream),
                                  std::istreambuf_iterator<char>()};
  try {
    return readWave(file);
  } catch (const std::runtime_error &error) {
    throw std::runtime_error(std::string(error.what()) + ": " + path);
  }
}

} // namespace openfranko::src::systems
