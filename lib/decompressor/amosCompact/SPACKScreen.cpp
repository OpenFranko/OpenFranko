#include "SPACKScreen.h"
#include "../helpers/helpers.h"
#include "Consts.h"
#include <stdexcept>

namespace openfranko::lib::decompressor::amosCompact {

SPACKScreen::SPACKScreen(const std::vector<uint8_t> &data) : m_data(data) {
  checkSize();
  parseHeader();
}

std::vector<uint8_t> SPACKScreen::getData() const { return {}; }

void SPACKScreen::checkSize() {
  if (m_data.size() <
      consts::SPACK_HEADER_SIZE + consts::PACKED_BITMAP_HEADER_SIZE) {
    throw std::runtime_error("File is too small to be a valid SPACK screen");
  }
}

void SPACKScreen::parseHeader() {
  m_header = headers::parseSPACKHeader(m_data);
}

} // namespace openfranko::lib::decompressor::amosCompact