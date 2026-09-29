#include "binary.h"

#include <stdexcept>

namespace openfranko::lib::binary {
namespace {

void requireBytes(const std::vector<uint8_t> &data, std::size_t pos,
                  std::size_t count) {
  if (pos > data.size() || count > data.size() - pos) {
    throw std::runtime_error("Unexpected end of binary data");
  }
}

} // namespace

BigEndianReader::BigEndianReader(const std::vector<uint8_t> &data)
    : m_data(data) {}

uint32_t BigEndianReader::readUint32(std::size_t pos) const {
  require(pos, 4);
  return (static_cast<uint32_t>(m_data[pos + 0]) << 24) |
         (static_cast<uint32_t>(m_data[pos + 1]) << 16) |
         (static_cast<uint32_t>(m_data[pos + 2]) << 8) |
         (static_cast<uint32_t>(m_data[pos + 3]) << 0);
}

uint16_t BigEndianReader::readUint16(std::size_t pos) const {
  require(pos, 2);
  return (static_cast<uint16_t>(m_data[pos + 0]) << 8) |
         (static_cast<uint16_t>(m_data[pos + 1]) << 0);
}

int32_t BigEndianReader::readInt32(std::size_t pos) const {
  return static_cast<int32_t>(readUint32(pos));
}

int16_t BigEndianReader::readInt16(std::size_t pos) const {
  return static_cast<int16_t>(readUint16(pos));
}

void BigEndianReader::require(std::size_t pos, std::size_t count) const {
  requireBytes(m_data, pos, count);
}

LittleEndianReader::LittleEndianReader(const std::vector<uint8_t> &data)
    : m_data(data) {}

uint32_t LittleEndianReader::readUint32(std::size_t pos) const {
  require(pos, 4);
  return static_cast<uint32_t>(m_data[pos]) |
         (static_cast<uint32_t>(m_data[pos + 1]) << 8) |
         (static_cast<uint32_t>(m_data[pos + 2]) << 16) |
         (static_cast<uint32_t>(m_data[pos + 3]) << 24);
}

uint16_t LittleEndianReader::readUint16(std::size_t pos) const {
  require(pos, 2);
  return static_cast<uint16_t>(m_data[pos]) |
         (static_cast<uint16_t>(m_data[pos + 1]) << 8);
}

void LittleEndianReader::require(std::size_t pos, std::size_t count) const {
  requireBytes(m_data, pos, count);
}

void pushBigEndian16(std::vector<uint8_t> &buf, uint16_t value) {
  buf.push_back(static_cast<uint8_t>(value >> 8));
  buf.push_back(static_cast<uint8_t>(value));
}

void pushBigEndian32(std::vector<uint8_t> &buf, uint32_t value) {
  buf.push_back(static_cast<uint8_t>(value >> 24));
  buf.push_back(static_cast<uint8_t>(value >> 16));
  buf.push_back(static_cast<uint8_t>(value >> 8));
  buf.push_back(static_cast<uint8_t>(value));
}

void pushLittleEndian16(std::vector<uint8_t> &buf, uint16_t value) {
  buf.push_back(static_cast<uint8_t>(value));
  buf.push_back(static_cast<uint8_t>(value >> 8));
}

void pushLittleEndian32(std::vector<uint8_t> &buf, uint32_t value) {
  buf.push_back(static_cast<uint8_t>(value));
  buf.push_back(static_cast<uint8_t>(value >> 8));
  buf.push_back(static_cast<uint8_t>(value >> 16));
  buf.push_back(static_cast<uint8_t>(value >> 24));
}

void writeLittleEndian16(std::vector<uint8_t> &buf, std::size_t pos,
                         uint16_t value) {
  requireBytes(buf, pos, 2);
  buf[pos] = static_cast<uint8_t>(value);
  buf[pos + 1] = static_cast<uint8_t>(value >> 8);
}

void writeLittleEndian32(std::vector<uint8_t> &buf, std::size_t pos,
                         uint32_t value) {
  requireBytes(buf, pos, 4);
  buf[pos] = static_cast<uint8_t>(value);
  buf[pos + 1] = static_cast<uint8_t>(value >> 8);
  buf[pos + 2] = static_cast<uint8_t>(value >> 16);
  buf[pos + 3] = static_cast<uint8_t>(value >> 24);
}

void padTo16(std::vector<uint8_t> &buf) {
  while (buf.size() % 16 != 0) {
    buf.push_back(0);
  }
}

} // namespace openfranko::lib::binary
