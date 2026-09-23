#include "bitmapUnpack.h"
#include "../Consts.h"
#include "BitReader.h"
#include "ByteReader.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>

namespace openfranko::lib::converter::amosCompact::detail {

namespace {

void mainDecompression(UnpackedBitmap &bitmap,
                       const headers::BitmapHeader &header, ByteReader &bytes1,
                       ByteReader &bytes2, BitReader &pointerBits) {
  const size_t lineSize = header.gridX;
  const size_t heightLines = bitmap.height;

  uint8_t mask = bytes2.read();
  uint8_t val = bytes1.read();

  if (pointerBits.read()) {
    mask = bytes2.read();
  }

  int maskBit = 7;

  for (int plane = 0; plane < bitmap.numberOfBitplanes; plane++) {
    uint8_t *planeData = bitmap.bitplaneData[plane].data();

    for (int tileRow = 0; tileRow < header.gridY; tileRow++) {
      for (int tileCol = 0; tileCol < header.gridX; tileCol++) {
        for (int row = 0; row < header.tileHeight; row++) {
          if ((mask >> maskBit) & 1) {
            val = bytes1.read();
          }

          const size_t outY =
              static_cast<size_t>(tileRow) * header.tileHeight + row;
          const size_t outX = tileCol;

          if (outY < heightLines && outX < lineSize) {
            planeData[outY * lineSize + outX] = val;
          }

          if (--maskBit < 0) {
            maskBit = 7;
            if (pointerBits.read()) {
              mask = bytes2.read();
            }
          }
        }
      }
    }
  }
}

void unpackChunkyPixels(UnpackedBitmap &bitmap) {
  size_t totalPixels = static_cast<size_t>(bitmap.width) * bitmap.height;
  uint16_t widthByBytes = bitmap.width / 8;

  bitmap.chunkyPixels.assign(totalPixels, 0);

  for (size_t y = 0; y < bitmap.height; y++) {
    for (size_t x = 0; x < widthByBytes; x++) {
      uint8_t planeBytes[consts::MAX_SUPPORTED_BITPLANES];
      for (int p = 0; p < bitmap.numberOfBitplanes; p++) {
        planeBytes[p] = bitmap.bitplaneData[p][y * widthByBytes + x];
      }

      for (int bit = 7; bit >= 0; bit--) {
        int shift = 7 - bit;
        uint8_t pixelValue = 0;
        for (int p = 0; p < bitmap.numberOfBitplanes; p++) {
          pixelValue |=
              static_cast<uint8_t>(((planeBytes[p] >> shift) & 1) << p);
        }
        bitmap
            .chunkyPixels[y * bitmap.width + x * 8 + static_cast<size_t>(bit)] =
            pixelValue;
      }
    }
  }
}

} // namespace

UnpackedBitmap bitmapUnpack(const std::vector<uint8_t> &packedData,
                            const headers::BitmapHeader &header,
                            const std::vector<uint16_t> &palette) {
  const size_t widthFull = static_cast<size_t>(header.gridX) * 8;
  const size_t heightFull =
      static_cast<size_t>(header.gridY) * header.tileHeight;

  if (widthFull == 0 || heightFull == 0) {
    throw std::runtime_error("Bitmap has zero dimensions");
  }
  if (widthFull > consts::MAX_BITMAP_DIMENSION ||
      heightFull > consts::MAX_BITMAP_DIMENSION) {
    throw std::runtime_error(
        "Bitmap dimensions out of range: " + std::to_string(widthFull) + "x" +
        std::to_string(heightFull));
  }
  if (widthFull * heightFull > consts::MAX_BITMAP_PIXELS) {
    throw std::runtime_error(
        "Bitmap is implausibly large: " + std::to_string(widthFull) + "x" +
        std::to_string(heightFull));
  }

  const uint16_t widthInPixels = static_cast<uint16_t>(widthFull);
  const uint16_t heightInLines = static_cast<uint16_t>(heightFull);
  const size_t planeSize = static_cast<size_t>(header.gridX) * heightInLines;

  size_t byteTable1Pointer = consts::PACKED_BITMAP_HEADER_SIZE;
  size_t byteTable2Pointer = header.offsetToByteTable2;
  size_t bitstreamPointer = header.offsetToPointerBitstream;

  if (packedData.size() <= bitstreamPointer ||
      packedData.size() <= byteTable2Pointer) {
    throw std::runtime_error("Packed data is too small to contain bitstream");
  }

  if (header.numberOfBitplanes == 0 ||
      header.numberOfBitplanes > consts::MAX_SUPPORTED_BITPLANES) {
    throw std::runtime_error("Unsupported bitplane count: " +
                             std::to_string(header.numberOfBitplanes));
  }

  UnpackedBitmap bitmap;
  bitmap.width = widthInPixels;
  bitmap.height = heightInLines;
  bitmap.numberOfBitplanes = header.numberOfBitplanes;
  bitmap.bytesPerPlane = planeSize;
  size_t paletteCopySize =
      std::min(palette.size() * sizeof(uint16_t), sizeof(bitmap.palette));
  std::memcpy(bitmap.palette, palette.data(), paletteCopySize);
  bitmap.bitplaneData.resize(bitmap.numberOfBitplanes);
  for (int p = 0; p < bitmap.numberOfBitplanes; p++) {
    bitmap.bitplaneData[p].assign(planeSize, 0);
  }

  ByteReader bytes1(packedData, byteTable1Pointer);
  ByteReader bytes2(packedData, byteTable2Pointer);
  BitReader bitReader(packedData, bitstreamPointer);

  mainDecompression(bitmap, header, bytes1, bytes2, bitReader);
  unpackChunkyPixels(bitmap);

  return bitmap;
}

} // namespace openfranko::lib::converter::amosCompact::detail