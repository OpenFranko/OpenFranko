#include "headers.h"
#include "../helpers/helpers.h"
#include "Consts.h"

namespace openfranko::lib::decompressor::amosCompact::headers {

SPACKHeader parseSPACKHeader(const std::vector<uint8_t> &data) {
  SPACKHeader header;
  header.screenWidth = helpers::readUint16BigEndian(data, 4);
  header.screenHeight = helpers::readUint16BigEndian(data, 6);
  header.windowX = helpers::readUint16BigEndian(data, 8);
  header.windowY = helpers::readUint16BigEndian(data, 10);
  header.windowWidth = helpers::readUint16BigEndian(data, 12);
  header.windowHeight = helpers::readUint16BigEndian(data, 14);
  header.viewX = helpers::readUint16BigEndian(data, 16);
  header.viewY = helpers::readUint16BigEndian(data, 18);
  header.displayModeFlags = helpers::readUint16BigEndian(data, 20);
  header.numberOfColors = helpers::readUint16BigEndian(data, 22);
  header.numberOfBitplanes = helpers::readUint16BigEndian(data, 24);

  for (size_t i = 0; i < consts::SPACK_PALETTE_SIZE; ++i) {
    uint16_t color =
        helpers::readUint16BigEndian(data, consts::SPACK_HEADER_SIZE + i * 2);
    header.amigaPalette.push_back(color);
  }
  return header;
}

BitmapHeader parseBitmapHeader(const std::vector<uint8_t> &data) {
  BitmapHeader header;
  header.xOffset = helpers::readInt16BigEndian(data, 4);
  header.yOffset = helpers::readInt16BigEndian(data, 6);
  header.bytesWidth = helpers::readUint16BigEndian(data, 8);
  header.rowsHeight = helpers::readUint16BigEndian(data, 10);
  header.tileHeight = helpers::readUint16BigEndian(data, 12);
  header.numberOfBitplanes = helpers::readUint16BigEndian(data, 14);
  header.offsetToByteTable = helpers::readUint32BigEndian(data, 16);
  header.offsetToPointerTable = helpers::readUint32BigEndian(data, 20);
  return header;
}

} // namespace openfranko::lib::decompressor::amosCompact::headers