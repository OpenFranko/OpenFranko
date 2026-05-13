#include "helpers.h"

namespace openfranko::lib::helpers {

uint32_t readUint32BigEndian(const std::vector<uint8_t> &data, size_t pos) {
  return (static_cast<uint32_t>(data[pos + 0]) << 24) |
         (static_cast<uint32_t>(data[pos + 1]) << 16) |
         (static_cast<uint32_t>(data[pos + 2]) << 8) |
         (static_cast<uint32_t>(data[pos + 3]) << 0);
}

uint16_t readUint16BigEndian(const std::vector<uint8_t> &data, size_t pos) {
  return (static_cast<uint16_t>(data[pos + 0]) << 8) |
         (static_cast<uint16_t>(data[pos + 1]) << 0);
}

int16_t readInt16BigEndian(const std::vector<uint8_t> &data, size_t pos) {
  return static_cast<int16_t>(readUint16BigEndian(data, pos));
}

uint32_t readUint32LittleEndian(const std::vector<uint8_t> &data, size_t pos) {
  return static_cast<uint32_t>(data[pos]) |
         (static_cast<uint32_t>(data[pos + 1]) << 8) |
         (static_cast<uint32_t>(data[pos + 2]) << 16) |
         (static_cast<uint32_t>(data[pos + 3]) << 24);
}

uint16_t readUint16LittleEndian(const std::vector<uint8_t> &data, size_t pos) {
  return static_cast<uint16_t>(data[pos]) |
         (static_cast<uint16_t>(data[pos + 1]) << 8);
}

void pushBigEndian16(std::vector<uint8_t> &buf, uint16_t v) {
  buf.push_back(static_cast<uint8_t>(v >> 8));
  buf.push_back(static_cast<uint8_t>(v));
}

void pushBigEndian32(std::vector<uint8_t> &buf, uint32_t v) {
  buf.push_back(static_cast<uint8_t>(v >> 24));
  buf.push_back(static_cast<uint8_t>(v >> 16));
  buf.push_back(static_cast<uint8_t>(v >> 8));
  buf.push_back(static_cast<uint8_t>(v));
}

void pushLittleEndian16(std::vector<uint8_t> &buf, uint16_t v) {
  buf.push_back(static_cast<uint8_t>(v));
  buf.push_back(static_cast<uint8_t>(v >> 8));
}

void pushLittleEndian32(std::vector<uint8_t> &buf, uint32_t v) {
  buf.push_back(static_cast<uint8_t>(v));
  buf.push_back(static_cast<uint8_t>(v >> 8));
  buf.push_back(static_cast<uint8_t>(v >> 16));
  buf.push_back(static_cast<uint8_t>(v >> 24));
}

void padTo16(std::vector<uint8_t> &buf) {
  while (buf.size() % 16 != 0) {
    buf.push_back(0);
  }
}

} // namespace openfranko::lib::helpers