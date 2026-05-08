#include "backwardLZ77.h"
#include "BitReader.h"
#include "Consts.h"
#include "helpers.h"
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace openfranko::lib::decompressor::backwardLZ77 {

namespace {

void applyMatch(std::vector<uint8_t> &out, size_t &writePtr, size_t offset,
                int length) {
  for (int i = 0; i < length && writePtr > 0; ++i) {
    writePtr--;
    size_t sourcePos = writePtr + offset;
    out[writePtr] = (sourcePos < out.size()) ? out[sourcePos] : 0;
  }
}

void applyLiteralRun(std::vector<uint8_t> &out, size_t &writePtr,
                     BitReader &reader, int count) {
  for (int i = 0; i < count && writePtr > 0; ++i) {
    writePtr--;
    out[writePtr] = reader.readRawByte();
  }
}

void processDecompression(BitReader &reader, std::vector<uint8_t> &out) {
  size_t writePtr = out.size();

  while (writePtr > 0) {
    bool isComplexCommand = reader.getBit();

    if (isComplexCommand) {
      uint32_t type = reader.getBits(2);

      if (type < 2) { // Short Match Type 1
        applyMatch(out, writePtr, reader.getBits(9 + type), type + 2);
      } else if (type == 2) { // Long Match
        int length = reader.getBits(8);
        applyMatch(out, writePtr, reader.getBits(12), length);
      } else { // Long Literal Run
        applyLiteralRun(out, writePtr, reader, reader.getBits(8) + 8);
      }
    } else {
      bool isShortMatch = reader.getBit();

      if (isShortMatch) { // Short Match Type 0
        applyMatch(out, writePtr, reader.getBits(8), 1);
      } else { // Short Literal Run
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

  const size_t footerStart = compressedData.size() - consts::FOOTER_SIZE;
  const size_t payloadEnd = footerStart + 8;

  uint32_t unpackedSize =
      helpers::readUint32BigEndian(compressedData, footerStart + 8);
  uint32_t xorChecksum =
      helpers::readUint32BigEndian(compressedData, footerStart + 4);
  uint32_t initialBits =
      helpers::readUint32BigEndian(compressedData, footerStart + 0);

  if (unpackedSize == 0)
    return {};

  std::vector<uint8_t> out(unpackedSize);

  BitReader reader(compressedData, footerStart, initialBits,
                   xorChecksum ^ initialBits);

  processDecompression(reader, out);

  if (!reader.verifyChecksum()) {
    throw std::runtime_error("Decompression failed: XOR checksum mismatch");
  }

  return out;
}

} // namespace openfranko::lib::decompressor::backwardLZ77