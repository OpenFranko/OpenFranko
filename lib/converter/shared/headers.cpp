#include "headers.h"
#include "../../helpers/helpers.h"
#include "../amosCompact/Consts.h"

namespace openfranko::lib::converter::headers {

SPACKHeader parseSPACKHeader(const std::vector<uint8_t> &data) {
  helpers::BigEndianReader reader(data);

  SPACKHeader header;
  header.screenWidth = reader.readUint16(4);
  header.screenHeight = reader.readUint16(6);
  header.windowX = reader.readUint16(8);
  header.windowY = reader.readUint16(10);
  header.windowWidth = reader.readUint16(12);
  header.windowHeight = reader.readUint16(14);
  header.viewX = reader.readUint16(16);
  header.viewY = reader.readUint16(18);
  header.displayModeFlags = reader.readUint16(20);
  header.numberOfColors = reader.readUint16(22);
  header.numberOfBitplanes = reader.readUint16(24);

  for (size_t i = 0; i < amosCompact::consts::SPACK_PALETTE_SIZE; ++i) {
    header.amigaPalette[i] = reader.readUint16(26 + i * 2);
  }
  return header;
}

BitmapHeader parseBitmapHeader(const std::vector<uint8_t> &data) {
  helpers::BigEndianReader reader(data);

  BitmapHeader header;
  header.xOffset = reader.readInt16(4);
  header.yOffset = reader.readInt16(6);
  header.gridX = reader.readUint16(8);
  header.gridY = reader.readUint16(10);
  header.tileHeight = reader.readUint16(12);
  header.numberOfBitplanes = reader.readUint16(14);
  header.offsetToByteTable2 = reader.readUint32(16);
  header.offsetToPointerBitstream = reader.readUint32(20);
  return header;
}

} // namespace openfranko::lib::converter::headers
