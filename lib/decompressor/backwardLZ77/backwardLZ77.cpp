#include "backwardLZ77.h"

#include "../../binary/binary.h"
#include "consts.h"
#include "detail/BitReader.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace openfranko::lib::decompressor::backwardLZ77 {
namespace {

constexpr std::size_t TRAILER_CHECKSUM_OFFSET = 4;
constexpr std::size_t TRAILER_UNPACKED_SIZE_OFFSET = 8;

void applyMatch(std::vector<uint8_t> &out, std::size_t &writePos,
                std::size_t offset, int count, std::size_t outputSize) {
  for (int i = 0; i < count && writePos > 0; ++i) {
    --writePos;
    std::size_t sourcePos = writePos + offset;
    out[writePos] = (sourcePos < outputSize) ? out[sourcePos] : 0;
  }
}

void applyLiteralRun(std::vector<uint8_t> &out, std::size_t &writePos,
                     detail::BitReader &reader, int count) {
  for (int i = 0; i < count && writePos > 0; ++i) {
    --writePos;
    out[writePos] = reader.readRawByte();
  }
}

void processDecompression(detail::BitReader &reader, std::vector<uint8_t> &out,
                          std::size_t unpackedSize) {
  std::size_t writePos = unpackedSize;

  while (writePos > 0) {
    bool isComplexCommand = reader.readBit();

    if (isComplexCommand) {
      uint32_t type = reader.readBits(2);

      if (type < 2) {
        applyMatch(out, writePos, reader.readBits(9 + type), type + 3,
                   unpackedSize);
      } else if (type == 2) {
        int length = reader.readBits(8);
        applyMatch(out, writePos, reader.readBits(12), length + 1,
                   unpackedSize);
      } else {
        applyLiteralRun(out, writePos, reader, reader.readBits(8) + 9);
      }
    } else {
      bool isShortMatch = reader.readBit();

      if (isShortMatch) {
        applyMatch(out, writePos, reader.readBits(8), 2, unpackedSize);
      } else {
        applyLiteralRun(out, writePos, reader, reader.readBits(3) + 1);
      }
    }
  }
}

} // namespace

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

  const std::size_t trailerStart = stream.size() - consts::TRAILER_SIZE;
  binary::BigEndianReader trailerReader(stream);

  uint32_t unpackedSize =
      trailerReader.readUint32(trailerStart + TRAILER_UNPACKED_SIZE_OFFSET);
  uint32_t xorChecksum =
      trailerReader.readUint32(trailerStart + TRAILER_CHECKSUM_OFFSET);
  uint32_t initialBits = trailerReader.readUint32(trailerStart);

  if (unpackedSize == 0) {
    throw std::runtime_error("Unpacked size is zero");
  }

  std::vector<uint8_t> out(unpackedSize, 0);

  detail::BitReader reader(stream, trailerStart, initialBits,
                           xorChecksum ^ initialBits);

  processDecompression(reader, out, unpackedSize);

  if (!reader.verifyChecksum()) {
    throw std::runtime_error("Decompression failed: XOR checksum mismatch");
  }

  return out;
}

} // namespace openfranko::lib::decompressor::backwardLZ77
