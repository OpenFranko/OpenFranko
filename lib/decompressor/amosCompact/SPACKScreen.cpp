#include "SPACKScreen.h"
#include "../helpers/helpers.h"
#include "Consts.h"
#include <stdexcept>

namespace openfranko::lib::decompressor::amosCompact {

SPACKScreen::SPACKScreen(const std::vector<uint8_t> &data) : m_data(data) {
  checkSize();
  parseHeaders();
}

std::vector<uint8_t> SPACKScreen::getData() const { return {}; }

void SPACKScreen::checkSize() {
  if (m_data.size() <
      consts::SPACK_HEADER_SIZE + consts::PACKED_BITMAP_HEADER_SIZE) {
    throw std::runtime_error("File is too small to be a valid SPACK screen");
  }
}

void SPACKScreen::parseHeaders() {
  m_spackHeader = headers::parseSPACKHeader(m_data);
  m_bitmapData = std::vector<uint8_t>(
      m_data.begin() + consts::SPACK_HEADER_SIZE, m_data.end());
  m_bitmapHeader = headers::parseBitmapHeader(m_bitmapData);
}

} // namespace openfranko::lib::decompressor::amosCompact