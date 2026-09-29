#include "headers.h"
#include "../../binary/binary.h"

namespace openfranko::lib::converter::headers {

namespace {

constexpr size_t SPACK_SCREEN_WIDTH_OFFSET = 4;
constexpr size_t SPACK_SCREEN_HEIGHT_OFFSET = 6;
constexpr size_t SPACK_WINDOW_X_OFFSET = 8;
constexpr size_t SPACK_WINDOW_Y_OFFSET = 10;
constexpr size_t SPACK_WINDOW_WIDTH_OFFSET = 12;
constexpr size_t SPACK_WINDOW_HEIGHT_OFFSET = 14;
constexpr size_t SPACK_VIEW_X_OFFSET = 16;
constexpr size_t SPACK_VIEW_Y_OFFSET = 18;
constexpr size_t SPACK_DISPLAY_MODE_FLAGS_OFFSET = 20;
constexpr size_t SPACK_COLORS_OFFSET = 22;
constexpr size_t SPACK_BITPLANES_OFFSET = 24;
constexpr size_t SPACK_PALETTE_OFFSET = 26;

constexpr size_t BITMAP_X_OFFSET_OFFSET = 4;
constexpr size_t BITMAP_Y_OFFSET_OFFSET = 6;
constexpr size_t BITMAP_GRID_X_OFFSET = 8;
constexpr size_t BITMAP_GRID_Y_OFFSET = 10;
constexpr size_t BITMAP_TILE_HEIGHT_OFFSET = 12;
constexpr size_t BITMAP_BITPLANES_OFFSET = 14;
constexpr size_t BITMAP_BYTE_TABLE2_POINTER_OFFSET = 16;
constexpr size_t BITMAP_BITSTREAM_POINTER_OFFSET = 20;

} // namespace

SpackHeader parseSpackHeader(const std::vector<uint8_t> &data) {
  binary::BigEndianReader reader(data);

  SpackHeader header;
  header.screenWidth = reader.readUint16(SPACK_SCREEN_WIDTH_OFFSET);
  header.screenHeight = reader.readUint16(SPACK_SCREEN_HEIGHT_OFFSET);
  header.windowX = reader.readUint16(SPACK_WINDOW_X_OFFSET);
  header.windowY = reader.readUint16(SPACK_WINDOW_Y_OFFSET);
  header.windowWidth = reader.readUint16(SPACK_WINDOW_WIDTH_OFFSET);
  header.windowHeight = reader.readUint16(SPACK_WINDOW_HEIGHT_OFFSET);
  header.viewX = reader.readUint16(SPACK_VIEW_X_OFFSET);
  header.viewY = reader.readUint16(SPACK_VIEW_Y_OFFSET);
  header.displayModeFlags = reader.readUint16(SPACK_DISPLAY_MODE_FLAGS_OFFSET);
  header.numberOfColors = reader.readUint16(SPACK_COLORS_OFFSET);
  header.numberOfBitplanes = reader.readUint16(SPACK_BITPLANES_OFFSET);

  for (size_t i = 0; i < SPACK_COLOR_COUNT; i++) {
    header.amigaPalette[i] = reader.readUint16(SPACK_PALETTE_OFFSET + i * 2);
  }
  return header;
}

BitmapHeader parseBitmapHeader(const std::vector<uint8_t> &data) {
  binary::BigEndianReader reader(data);

  BitmapHeader header;
  header.xOffset = reader.readInt16(BITMAP_X_OFFSET_OFFSET);
  header.yOffset = reader.readInt16(BITMAP_Y_OFFSET_OFFSET);
  header.gridX = reader.readUint16(BITMAP_GRID_X_OFFSET);
  header.gridY = reader.readUint16(BITMAP_GRID_Y_OFFSET);
  header.tileHeight = reader.readUint16(BITMAP_TILE_HEIGHT_OFFSET);
  header.numberOfBitplanes = reader.readUint16(BITMAP_BITPLANES_OFFSET);
  header.offsetToByteTable2 =
      reader.readUint32(BITMAP_BYTE_TABLE2_POINTER_OFFSET);
  header.offsetToPointerBitstream =
      reader.readUint32(BITMAP_BITSTREAM_POINTER_OFFSET);
  return header;
}

} // namespace openfranko::lib::converter::headers
