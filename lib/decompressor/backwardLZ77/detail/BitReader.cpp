#include "BitReader.h"
#include "../../../binary/binary.h"
#include "../consts.h"

#include <stdexcept>

namespace openfranko::lib::decompressor::backwardLZ77::detail {

BitReader::BitReader(const std::vector<uint8_t> &data, size_t endPos,
                     uint32_t initialBuffer, uint32_t xorChecksum)
    : m_data(data), m_readPos(endPos), m_buffer(initialBuffer),
      m_checksum(xorChecksum) {}

uint8_t BitReader::readRawByte() { return static_cast<uint8_t>(readBits(8)); }

uint32_t BitReader::readBit() {
  uint32_t bit = m_buffer & 1;
  m_buffer >>= 1;

  if (m_buffer == 0) {
    refill();
    return m_lastBitBeforeRefill;
  }
  return bit;
}

uint32_t BitReader::readBits(int count) {
  uint32_t result = 0;
  for (int i = 0; i < count; i++) {
    result = (result << 1) | readBit();
  }
  return result;
}

bool BitReader::verifyChecksum() const { return m_checksum == 0; }

void BitReader::refill() {
  if (m_readPos < 4) {
    throw std::runtime_error("Unexpected end of stream");
  }

  m_readPos -= 4;
  uint32_t nextWord = binary::BigEndianReader(m_data).readUint32(m_readPos);

  m_checksum ^= nextWord;
  m_lastBitBeforeRefill = nextWord & 1;
  m_buffer = (nextWord >> 1) | consts::BIT_SENTINEL;
}

} // namespace openfranko::lib::decompressor::backwardLZ77::detail
