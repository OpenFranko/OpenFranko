#include "headers.h"
#include "../../decompressor/helpers/helpers.h"
#include "../amosCompact/Consts.h"

namespace openfranko::lib::converter::headers {

SPACKHeader parseSPACKHeader(const std::vector<uint8_t> &data) {
  SPACKHeader header;
  header.screenWidth = decompressor::helpers::readUint16BigEndian(data, 4);
  header.screenHeight = decompressor::helpers::readUint16BigEndian(data, 6);
  header.windowX = decompressor::helpers::readUint16BigEndian(data, 8);
  header.windowY = decompressor::helpers::readUint16BigEndian(data, 10);
  header.windowWidth = decompressor::helpers::readUint16BigEndian(data, 12);
  header.windowHeight = decompressor::helpers::readUint16BigEndian(data, 14);
  header.viewX = decompressor::helpers::readUint16BigEndian(data, 16);
  header.viewY = decompressor::helpers::readUint16BigEndian(data, 18);
  header.displayModeFlags = decompressor::helpers::readUint16BigEndian(data, 20);
  header.numberOfColors = decompressor::helpers::readUint16BigEndian(data, 22);
  header.numberOfBitplanes = decompressor::helpers::readUint16BigEndian(data, 24);

  for (size_t i = 0; i < amosCompact::consts::SPACK_PALETTE_SIZE; ++i) {
    header.amigaPalette[i] = decompressor::helpers::readUint16BigEndian(data, 26 + i * 2);
  }
  return header;
}

BitmapHeader parseBitmapHeader(const std::vector<uint8_t> &data) {
  BitmapHeader header;
  header.xOffset = decompressor::helpers::readInt16BigEndian(data, 4);
  header.yOffset = decompressor::helpers::readInt16BigEndian(data, 6);
  header.gridX = decompressor::helpers::readUint16BigEndian(data, 8);
  header.gridY = decompressor::helpers::readUint16BigEndian(data, 10);
  header.tileHeight = decompressor::helpers::readUint16BigEndian(data, 12);
  header.numberOfBitplanes = decompressor::helpers::readUint16BigEndian(data, 14);
  header.offsetToByteTable2 = decompressor::helpers::readUint32BigEndian(data, 16);
  header.offsetToPointerBitstream = decompressor::helpers::readUint32BigEndian(data, 20);
  return header;
}

} // namespace openfranko::lib::converter::headers
