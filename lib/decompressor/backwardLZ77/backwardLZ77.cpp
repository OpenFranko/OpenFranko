#include "backwardLZ77.h"
#include "../../helpers/helpers.h"
#include "BitReader.h"
#include "Consts.h"
#include <algorithm>
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

  const size_t footerStart = compressedData.size() - consts::FOOTER_SIZE;

  uint32_t unpackedSize =
      helpers::readUint32BigEndian(compressedData, footerStart + 8);
  uint32_t xorChecksum =
      helpers::readUint32BigEndian(compressedData, footerStart + 4);
  uint32_t initialBits =
      helpers::readUint32BigEndian(compressedData, footerStart + 0);

  if (unpackedSize == 0) {
    throw std::runtime_error("Unpacked size is zero");
  }

  const size_t payloadSize =
      std::min<size_t>(footerStart, static_cast<size_t>(unpackedSize));
  std::vector<uint8_t> out(payloadSize + unpackedSize + 4096, 0);
  std::copy_n(compressedData.begin(), payloadSize, out.begin());

  BitReader reader(out, payloadSize, initialBits, xorChecksum ^ initialBits);

  processDecompression(reader, out, unpackedSize);

  if (!reader.verifyChecksum()) {
    throw std::runtime_error("Decompression failed: XOR checksum mismatch");
  }

  out.resize(unpackedSize);
  return out;
}

} // namespace openfranko::lib::decompressor::backwardLZ77