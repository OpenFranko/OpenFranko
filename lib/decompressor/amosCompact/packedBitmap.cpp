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
  m_header = headers::parseBitmapHeader(m_data);
}

} // namespace openfranko::lib::decompressor::amosCompact