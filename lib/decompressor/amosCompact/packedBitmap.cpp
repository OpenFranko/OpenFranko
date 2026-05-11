#include "packedBitmap.h"
#include "../helpers/helpers.h"
#include "Consts.h"
#include <stdexcept>

namespace openfranko::lib::decompressor::amosCompact {

PackedBitmap::PackedBitmap(const std::vector<uint8_t> &data) : m_data(data) {
  checkSize();
  parseHeader();
}

std::vector<uint8_t> PackedBitmap::getData() const { return {}; }

void PackedBitmap::checkSize() {
  if (m_data.size() < consts::PACKED_BITMAP_HEADER_SIZE) {
    throw std::runtime_error("File is too small to be a valid packed bitmap");
  }
}

void PackedBitmap::parseHeader() {
  m_header.xOffset = helpers::readInt16BigEndian(m_data, 4);
  m_header.yOffset = helpers::readInt16BigEndian(m_data, 6);
  m_header.bytesWidth = helpers::readUint16BigEndian(m_data, 8);
  m_header.rowsHeight = helpers::readUint16BigEndian(m_data, 10);
  m_header.tileHeight = helpers::readUint16BigEndian(m_data, 12);
  m_header.numberOfBitplanes = helpers::readUint16BigEndian(m_data, 14);
  m_header.offsetToByteTable = helpers::readUint32BigEndian(m_data, 16);
  m_header.offsetToPointerTable = helpers::readUint32BigEndian(m_data, 20);
}

} // namespace openfranko::lib::decompressor::amosCompact