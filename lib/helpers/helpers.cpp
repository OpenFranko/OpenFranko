#include "helpers.h"
#include <stdexcept>

namespace openfranko::lib::helpers {

namespace {

void requireBytes(const std::vector<uint8_t> &data, size_t pos, size_t count) {
  if (pos > data.size() || count > data.size() - pos) {
    throw std::runtime_error("Unexpected end of binary data");
  }
}

} // namespace

BigEndianReader::BigEndianReader(const std::vector<uint8_t> &data)
    : m_data(data) {}

uint32_t BigEndianReader::readUint32(size_t pos) const {
  require(pos, 4);
  return (static_cast<uint32_t>(m_data[pos + 0]) << 24) |
         (static_cast<uint32_t>(m_data[pos + 1]) << 16) |
         (static_cast<uint32_t>(m_data[pos + 2]) << 8) |
         (static_cast<uint32_t>(m_data[pos + 3]) << 0);
}

uint16_t BigEndianReader::readUint16(size_t pos) const {
  require(pos, 2);
  return (static_cast<uint16_t>(m_data[pos + 0]) << 8) |
         (static_cast<uint16_t>(m_data[pos + 1]) << 0);
}

int16_t BigEndianReader::readInt16(size_t pos) const {
  return static_cast<int16_t>(readUint16(pos));
}

void BigEndianReader::require(size_t pos, size_t count) const {
  requireBytes(m_data, pos, count);
}

LittleEndianReader::LittleEndianReader(const std::vector<uint8_t> &data)
    : m_data(data) {}

uint32_t LittleEndianReader::readUint32(size_t pos) const {
  require(pos, 4);
  return static_cast<uint32_t>(m_data[pos]) |
         (static_cast<uint32_t>(m_data[pos + 1]) << 8) |
         (static_cast<uint32_t>(m_data[pos + 2]) << 16) |
         (static_cast<uint32_t>(m_data[pos + 3]) << 24);
}

uint16_t LittleEndianReader::readUint16(size_t pos) const {
  require(pos, 2);
  return static_cast<uint16_t>(m_data[pos]) |
         (static_cast<uint16_t>(m_data[pos + 1]) << 8);
}

void LittleEndianReader::require(size_t pos, size_t count) const {
  requireBytes(m_data, pos, count);
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
