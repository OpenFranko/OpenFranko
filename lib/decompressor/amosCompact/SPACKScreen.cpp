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
  m_header.screenWidth = helpers::readUint16BigEndian(m_data, 4);
  m_header.screenHeight = helpers::readUint16BigEndian(m_data, 6);
  m_header.windowX = helpers::readUint16BigEndian(m_data, 8);
  m_header.windowY = helpers::readUint16BigEndian(m_data, 10);
  m_header.windowWidth = helpers::readUint16BigEndian(m_data, 12);
  m_header.windowHeight = helpers::readUint16BigEndian(m_data, 14);
  m_header.viewX = helpers::readUint16BigEndian(m_data, 16);
  m_header.viewY = helpers::readUint16BigEndian(m_data, 18);
  m_header.displayModeFlags = helpers::readUint16BigEndian(m_data, 20);
  m_header.numberOfColors = helpers::readUint16BigEndian(m_data, 22);
  m_header.numberOfBitplanes = helpers::readUint16BigEndian(m_data, 24);

  for (size_t i = 0; i < consts::SPACK_PALETTE_SIZE; ++i) {
    uint16_t color =
        helpers::readUint16BigEndian(m_data, consts::SPACK_HEADER_SIZE + i * 2);
    m_header.amigaPalette.push_back(color);
  }
}

} // namespace openfranko::lib::decompressor::amosCompact