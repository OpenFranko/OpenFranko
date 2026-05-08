#include "BitReader.h"
#include "Consts.h"
#include "helpers.h"
#include <stdexcept>

namespace openfranko::lib::decompressor::backwardLZ77 {

BitReader::BitReader(const std::vector<uint8_t> &data, size_t endPos,
                     uint32_t initialBuffer, uint32_t xorChecksum)
    : m_data(data), m_readPos(endPos), m_buffer(initialBuffer),
      m_checksum(xorChecksum) {}

uint8_t BitReader::readRawByte() {
  if (m_readPos == 0)
    throw std::runtime_error("Stream underflow");
  return m_data[--m_readPos];
}

uint32_t BitReader::getBit() {
  uint32_t bit = m_buffer & 1;
  m_buffer >>= 1;

  if (m_buffer == 0) {
    refill();
    return m_lastBitBeforeRefill;
  }
  return bit;
}

uint32_t BitReader::getBits(int count) {
  uint32_t result = 0;
  for (int i = 0; i < count; ++i) {
    result = (result << 1) | getBit();
  }
  return result;
}

bool BitReader::verifyChecksum() const { return m_checksum == 0; }

void BitReader::refill() {
  if (m_readPos < 4)
    throw std::runtime_error("Unexpected end of stream");

  m_readPos -= 4;
  uint32_t nextWord = helpers::readUint32BigEndian(m_data, m_readPos);

  m_checksum ^= nextWord;

  m_lastBitBeforeRefill = nextWord & 1;
  m_buffer = (nextWord >> 1) | consts::BIT_SENTINEL;
}

} // namespace openfranko::lib::decompressor::backwardLZ77