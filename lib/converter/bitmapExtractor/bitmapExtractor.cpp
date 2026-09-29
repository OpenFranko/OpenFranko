#include "bitmapExtractor.h"

#include "../../binary/binary.h"
#include "../../bmpWriter/bmpWriter.h"
#include "../amosCompact/decodeAmosBitmap.h"
#include "../gameData/gameData.h"
#include "../gameData/palettes.h"
#include "../headers/headers.h"

#include <algorithm>
#include <stdexcept>

namespace openfranko::lib::converter::bitmapExtractor {
namespace {

constexpr std::size_t MAGIC_SIZE = 4;
constexpr std::size_t LONG_ENTRY_SIZE = 4;
constexpr std::size_t WORD_ENTRY_SIZE = 2;
constexpr std::size_t TILE_HEADER_SIZE = 4;
constexpr std::size_t TILE_COUNT_OFFSET = 3;
constexpr std::size_t TILE_LENGTH_SIZE = 2;
constexpr uint16_t MIN_BITMAP_SIDE = 2;

std::vector<std::size_t> findBmCodeOffsets(const std::vector<uint8_t> &data) {
  std::vector<std::size_t> offsets;
  binary::BigEndianReader reader(data);
  for (std::size_t off = 0;
       off + headers::PACKED_BITMAP_HEADER_SIZE <= data.size(); off += 2) {
    if (reader.readUint32(off) == headers::AMOS_BMCODE) {
      offsets.push_back(off);
    }
  }
  return offsets;
}

bool hasMagicAt(const std::vector<uint8_t> &data, std::size_t offset,
                uint32_t magic, std::size_t headerSize) {
  return offset + headerSize <= data.size() &&
         binary::BigEndianReader(data).readUint32(offset) == magic;
}

bool isBitmapAt(const std::vector<uint8_t> &data, std::size_t offset) {
  return hasMagicAt(data, offset, headers::AMOS_BMCODE,
                    headers::PACKED_BITMAP_HEADER_SIZE);
}

bool isScreenAt(const std::vector<uint8_t> &data, std::size_t offset) {
  return hasMagicAt(data, offset, headers::SPACK_SCREEN_HEADER,
                    headers::SPACK_HEADER_SIZE);
}

std::vector<std::size_t> readBitmapTable(const std::vector<uint8_t> &data) {
  binary::BigEndianReader reader(data);
  for (std::size_t entrySize : {LONG_ENTRY_SIZE, WORD_ENTRY_SIZE}) {
    if (data.size() < entrySize) {
      continue;
    }
    auto entry = [&](std::size_t pos) -> std::size_t {
      return entrySize == LONG_ENTRY_SIZE ? reader.readUint32(pos)
                                          : reader.readUint16(pos);
    };
    const std::size_t first = entry(0);
    if (first == 0 || first % entrySize != 0 || !isBitmapAt(data, first)) {
      continue;
    }
    std::vector<std::size_t> offsets;
    for (std::size_t pos = 0; pos < first; pos += entrySize) {
      offsets.push_back(entry(pos));
    }
    return offsets;
  }
  return {};
}

std::vector<std::size_t> readTileChain(const std::vector<uint8_t> &data) {
  if (data.size() < TILE_HEADER_SIZE + TILE_LENGTH_SIZE) {
    throw std::runtime_error("Tile file is too small for its header");
  }
  const uint8_t count = data[TILE_COUNT_OFFSET];
  if (count == 0) {
    throw std::runtime_error("Tile file has no tiles");
  }

  binary::BigEndianReader reader(data);
  std::vector<std::size_t> offsets;
  std::size_t offset = TILE_HEADER_SIZE + TILE_LENGTH_SIZE;
  for (int i = 0; i < count; ++i) {
    if (!isBitmapAt(data, offset)) {
      throw std::runtime_error("Tile chain is broken at tile " +
                               std::to_string(i));
    }
    offsets.push_back(offset);
    offset += reader.readUint16(offset - TILE_LENGTH_SIZE);
  }
  return offsets;
}

bool isTileFile(const std::string &fileId) {
  return std::find(gameData::fileIds::TILE_FILES.begin(),
                   gameData::fileIds::TILE_FILES.end(),
                   gameData::version10Id(fileId)) !=
         gameData::fileIds::TILE_FILES.end();
}

std::vector<uint16_t> readSpackPalette(const std::vector<uint8_t> &data,
                                       std::size_t offset) {
  if (offset > data.size()) {
    throw std::runtime_error("SPACK palette offset is past the end of data");
  }
  std::vector<uint8_t> slice(data.begin() + static_cast<std::ptrdiff_t>(offset),
                             data.end());
  auto header = headers::parseSpackHeader(slice);
  return {std::begin(header.amigaPalette), std::end(header.amigaPalette)};
}

std::string skipReason(const amosCompact::DecodedImage &image) {
  if (image.pixels.empty()) {
    return "Bitmap decoded to an empty image";
  }
  return "Bitmap is only " + std::to_string(image.width) + "x" +
         std::to_string(image.height) + " pixels";
}

ExtractedBitmap convertBitmap(const std::vector<uint8_t> &data,
                              std::size_t offset,
                              const std::vector<uint16_t> &palette,
                              const std::string &name, bool skipTiny) {
  try {
    auto image = amosCompact::decodeAmosBitmap(data, offset);
    if (image.pixels.empty() ||
        (skipTiny &&
         (image.width < MIN_BITMAP_SIDE || image.height < MIN_BITMAP_SIDE))) {
      return {name, {}, skipReason(image)};
    }
    auto bmp = bmpWriter::pixelsToBmp(image.width, image.height,
                                      image.pixels.data(), palette.data(),
                                      static_cast<int>(palette.size()));
    return {name, std::move(bmp), {}};
  } catch (const std::exception &e) {
    return {name, {}, e.what()};
  }
}

std::vector<ExtractedBitmap> extractScCode(const std::vector<uint8_t> &data,
                                           const std::string &fileId) {
  const auto palette = readSpackPalette(data, 0);
  auto offsets = findBmCodeOffsets(data);

  std::vector<ExtractedBitmap> results;
  for (std::size_t i = 0; i < offsets.size(); ++i) {
    std::string name = (i == 0) ? fileId : fileId + "_" + std::to_string(i);
    results.push_back(convertBitmap(data, offsets[i], palette, name, false));
  }
  return results;
}

std::vector<ExtractedBitmap> extractTiles(const std::vector<uint8_t> &data,
                                          const std::string &fileId) {
  auto offsets = readTileChain(data);

  const std::vector<uint16_t> palette(gameData::palettes::LEVEL.begin(),
                                      gameData::palettes::LEVEL.end());
  std::vector<ExtractedBitmap> results;
  for (std::size_t i = 0; i < offsets.size(); ++i) {
    char name[32];
    snprintf(name, sizeof(name), "%s_%03zu", fileId.c_str(), i);
    results.push_back(convertBitmap(data, offsets[i], palette, name, false));
  }
  return results;
}

std::vector<ExtractedBitmap>
extractMultiBmCode(const std::vector<uint8_t> &data,
                   const std::string &fileId) {
  const auto palette = gameData::palettes::selectPalette(fileId);
  auto offsets = readBitmapTable(data);
  if (offsets.empty()) {
    offsets = findBmCodeOffsets(data);
  }

  std::vector<ExtractedBitmap> results;
  for (std::size_t i = 0; i < offsets.size(); ++i) {
    std::string name = (i == 0) ? fileId : fileId + "_" + std::to_string(i);
    results.push_back(convertBitmap(data, offsets[i], palette, name, true));
  }
  return results;
}

std::vector<ExtractedBitmap> extract0384(const std::vector<uint8_t> &data,
                                         const std::string &fileId) {
  std::vector<uint16_t> palette(gameData::palettes::HUD.begin(),
                                gameData::palettes::HUD.end());

  std::vector<ExtractedBitmap> results;
  binary::BigEndianReader reader(data);

  std::size_t firstImage = data.size();
  for (std::size_t pos = 0; pos + WORD_ENTRY_SIZE <= firstImage;
       pos += WORD_ENTRY_SIZE) {
    std::size_t offset = reader.readUint16(pos);
    const bool screen = isScreenAt(data, offset);
    if (offset <= pos || (!screen && !isBitmapAt(data, offset))) {
      break;
    }
    firstImage = std::min(firstImage, offset);
    if (screen) {
      palette = readSpackPalette(data, offset);
      offset += headers::SPACK_HEADER_SIZE;
    }
    const std::size_t index = results.size();
    std::string name =
        (index == 0) ? fileId : fileId + "_" + std::to_string(index);
    results.push_back(convertBitmap(data, offset, palette, name, true));
  }
  return results;
}

} // namespace

std::vector<ExtractedBitmap> extract(const std::vector<uint8_t> &data,
                                     const std::string &fileId) {
  if (data.size() < MAGIC_SIZE) {
    throw std::runtime_error("Data too small to extract bitmaps");
  }

  binary::BigEndianReader reader(data);
  uint32_t magic = reader.readUint32(0);

  const std::string_view role = gameData::version10Id(fileId);

  if (role == gameData::fileIds::MULTI_PALETTE_BITMAP) {
    return extract0384(data, fileId);
  }

  if (magic == headers::SPACK_SCREEN_HEADER &&
      role != gameData::fileIds::CEMETERY_PICTURE) {
    return extractScCode(data, fileId);
  }

  if (isTileFile(fileId)) {
    return extractTiles(data, fileId);
  }

  return extractMultiBmCode(data, fileId);
}

} // namespace openfranko::lib::converter::bitmapExtractor
