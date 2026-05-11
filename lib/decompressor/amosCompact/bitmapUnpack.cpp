#include "bitmapUnpack.h"
#include "BitReader.h"
#include "ByteReader.h"
#include "Consts.h"
#include "unpackedBitmap.h"
#include <stdexcept>

namespace openfranko::lib::decompressor::amosCompact {

std::vector<uint8_t> bitmapUnpack(const std::vector<uint8_t> &packedData,
                                  const headers::BitmapHeader &header,
                                  const std::vector<uint16_t> &palette) {
  const uint16_t widthInPixels = header.gridX * 8;
  const uint16_t heightInLines = header.gridY * header.tileHeight;
  const size_t planeSize = header.gridX * heightInLines;

  size_t byteTable1Pointer = consts::PACKED_BITMAP_HEADER_SIZE;
  size_t byteTable2Pointer = header.offsetToByteTable2;
  size_t bitstreamPointer = header.offsetToPointerBitstream;

  if (packedData.size() <= bitstreamPointer ||
      packedData.size() <= byteTable2Pointer) {
    throw std::runtime_error("Packed data is too small to contain bitstream");
  }

  UnpackedBitmap bitmap;
  bitmap.width = widthInPixels;
  bitmap.height = heightInLines;
  bitmap.numberOfBitplanes = header.numberOfBitplanes;
  bitmap.bytesPerPlane = planeSize;
  bitmap.palette = palette;

  for (int p = 0; p < bitmap.numberOfBitplanes; p++) {
    bitmap.bitplaneData[p] = static_cast<uint8_t *>(calloc(1, planeSize));
  }

  ByteReader bytes1(packedData, byteTable1Pointer);
  ByteReader bytes2(packedData, byteTable2Pointer);
  BitReader bitReader(packedData, bitstreamPointer);

  return {};
}

} // namespace openfranko::lib::decompressor::amosCompact