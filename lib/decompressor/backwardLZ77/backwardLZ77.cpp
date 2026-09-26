#include "backwardLZ77.h"
#include "../../helpers/helpers.h"
#include "BitReader.h"
#include "Consts.h"
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace openfranko::lib::decompressor::backwardLZ77 {

namespace {

void applyMatch(std::vector<uint8_t> &out, size_t &writePtr, size_t offset,
                int count, size_t outputSize) {
  for (int i = 0; i < count && writePtr > 0; ++i) {
    writePtr--;
    size_t sourcePos = writePtr + offset;
    out[writePtr] = (sourcePos < outputSize) ? out[sourcePos] : 0;
  }
}

void applyLiteralRun(std::vector<uint8_t> &out, size_t &writePtr,
                     BitReader &reader, int count) {
  for (int i = 0; i < count && writePtr > 0; ++i) {
    writePtr--;
    out[writePtr] = reader.readRawByte();
  }
}

void processDecompression(BitReader &reader, std::vector<uint8_t> &out,
                          size_t unpackedSize) {
  size_t writePtr = unpackedSize;

  while (writePtr > 0) {
    bool isComplexCommand = reader.getBit();

    if (isComplexCommand) {
      uint32_t type = reader.getBits(2);

      if (type < 2) {
        applyMatch(out, writePtr, reader.getBits(9 + type), type + 3,
                   unpackedSize);
      } else if (type == 2) {
        int length = reader.getBits(8);
        applyMatch(out, writePtr, reader.getBits(12), length + 1, unpackedSize);
      } else {
        applyLiteralRun(out, writePtr, reader, reader.getBits(8) + 9);
      }
    } else {
      bool isShortMatch = reader.getBit();

      if (isShortMatch) {
        applyMatch(out, writePtr, reader.getBits(8), 2, unpackedSize);
      } else {
        applyLiteralRun(out, writePtr, reader, reader.getBits(3) + 1);
      }
    }
  }
}

} // anonymous namespace

std::vector<uint8_t> decompress(const std::vector<uint8_t> &compressedData) {
  if (compressedData.size() < consts::FOOTER_SIZE) {
    throw std::runtime_error("File too small to contain footer");
  }

  return decompressStream(std::vector<uint8_t>(
      compressedData.begin(),
      compressedData.end() -
          static_cast<std::ptrdiff_t>(consts::FILE_FOOTER_SIZE)));
}

std::vector<uint8_t> decompressStream(const std::vector<uint8_t> &stream) {
  if (stream.size() < consts::TRAILER_SIZE) {
    throw std::runtime_error("Stream too small to contain its trailer");
  }

  const size_t trailerStart = stream.size() - consts::TRAILER_SIZE;
  helpers::BigEndianReader trailerReader(stream);

  uint32_t unpackedSize = trailerReader.readUint32(trailerStart + 8);
  uint32_t xorChecksum = trailerReader.readUint32(trailerStart + 4);
  uint32_t initialBits = trailerReader.readUint32(trailerStart + 0);

  if (unpackedSize == 0) {
    throw std::runtime_error("Unpacked size is zero");
  }

  std::vector<uint8_t> out(unpackedSize, 0);

  BitReader reader(stream, trailerStart, initialBits,
                   xorChecksum ^ initialBits);

  processDecompression(reader, out, unpackedSize);

  if (!reader.verifyChecksum()) {
    throw std::runtime_error("Decompression failed: XOR checksum mismatch");
  }

  return out;
}

} // namespace openfranko::lib::decompressor::backwardLZ77
