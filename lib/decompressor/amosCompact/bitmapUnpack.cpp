#include "bitmapUnpack.h"
#include "BitReader.h"
#include "ByteReader.h"
#include "Consts.h"
#include <cstring>
#include <stdexcept>

namespace openfranko::lib::decompressor::amosCompact {

namespace {

void mainDecompression(UnpackedBitmap &bitmap,
                       const headers::BitmapHeader &header, ByteReader &bytes1,
                       ByteReader &bytes2, BitReader &pointerBits) {
  uint16_t lineSize = header.gridX;
  uint16_t heightLines = header.gridY * header.tileHeight;

  uint8_t mask = bytes2.read();
  uint8_t val = bytes1.read();

  if (pointerBits.read()) {
    mask = bytes2.read();
  }

  int maskBit = 7;

  for (int plane; plane < bitmap.numberOfBitplanes; plane++) {
    uint8_t *planeData = bitmap.bitplaneData[plane];

    for (int tileRow = 0; tileRow < header.gridY; tileRow++) {
      for (int tileCol = 0; tileCol < header.gridX; tileCol++) {
        for (int row; row < header.tileHeight; row++) {
          if (mask >> maskBit & 1) {
            val = bytes1.read();
          }

          int outX = tileRow * header.tileHeight + row;
          int outY = tileCol;

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
  size_t totalPixels = bitmap.width * bitmap.height;
  uint16_t widthByBytes = bitmap.width / 8;

  if (!bitmap.chunkyPixels) {
    bitmap.chunkyPixels = static_cast<uint8_t *>(calloc(1, totalPixels));
  } else {
    std::memset(bitmap.chunkyPixels, 0, totalPixels);
  }

  for (int y = 0; y < bitmap.height; y++) {
    for (int x = 0; x < widthByBytes; x++) {
      uint8_t planeBytes[consts::MAX_SUPPORTED_BITPLANES];
      for (int p = 0; p < bitmap.numberOfBitplanes; p++) {
        planeBytes[p] = bitmap.bitplaneData[p][y * widthByBytes + x];
      }

      for (int bit = 7; bit >= 0; bit--) {
        int shift = 7 - bit;
        uint8_t pixelValue = 0;
        for (int p = 0; p < bitmap.numberOfBitplanes; p++) {
          pixelValue |= ((planeBytes[p] >> shift) & 1) << p;
        }
        bitmap.chunkyPixels[y * bitmap.width + x * 8 + bit] = pixelValue;
      }
    }
  }
}

} // namespace

UnpackedBitmap bitmapUnpack(const std::vector<uint8_t> &packedData,
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
  std::memcpy(bitmap.palette, palette.data(), sizeof(bitmap.palette));
  for (int p = 0; p < bitmap.numberOfBitplanes; p++) {
    bitmap.bitplaneData[p] = static_cast<uint8_t *>(calloc(1, planeSize));
  }

  ByteReader bytes1(packedData, byteTable1Pointer);
  ByteReader bytes2(packedData, byteTable2Pointer);
  BitReader bitReader(packedData, bitstreamPointer);

  mainDecompression(bitmap, header, bytes1, bytes2, bitReader);
  unpackChunkyPixels(bitmap);

  return bitmap;
}

} // namespace openfranko::lib::decompressor::amosCompact