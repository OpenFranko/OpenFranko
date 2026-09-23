#include "bitmapExtractor.h"
#include "../../bmpWriter/bmpWriter.h"
#include "../../helpers/helpers.h"
#include "../amosCompact/Consts.h"
#include "../gameData/gameData.h"
#include "../shared/decodeImage.h"
#include "../shared/headers.h"
#include "../spriteSheet/Palettes.h"
#include <algorithm>
#include <stdexcept>

namespace openfranko::lib::converter::bitmapExtractor {

namespace amosConsts = amosCompact::consts;
namespace pal = spriteSheet::palettes;
using converter::decodeAmosBitmap;
using converter::DecodedImage;

namespace {

std::vector<size_t> findBMCodeOffsets(const std::vector<uint8_t> &data) {
  std::vector<size_t> offsets;
  helpers::BigEndianReader reader(data);
  for (size_t off = 0;
       off + amosConsts::PACKED_BITMAP_HEADER_SIZE <= data.size(); off += 2) {
    if (reader.readUint32(off) == amosConsts::AMOS_BMCODE) {
      offsets.push_back(off);
    }
  }
  return offsets;
}

bool hasMagicAt(const std::vector<uint8_t> &data, size_t offset, uint32_t magic,
                size_t headerSize) {
  return offset + headerSize <= data.size() &&
         helpers::BigEndianReader(data).readUint32(offset) == magic;
}

bool isBitmapAt(const std::vector<uint8_t> &data, size_t offset) {
  return hasMagicAt(data, offset, amosConsts::AMOS_BMCODE,
                    amosConsts::PACKED_BITMAP_HEADER_SIZE);
}

bool isScreenAt(const std::vector<uint8_t> &data, size_t offset) {
  return hasMagicAt(data, offset, amosConsts::SPACK_SCREEN_HEADER,
                    amosConsts::SPACK_HEADER_SIZE);
}

std::vector<size_t> readBitmapTable(const std::vector<uint8_t> &data) {
  helpers::BigEndianReader reader(data);
  for (size_t entrySize : {size_t{4}, size_t{2}}) {
    if (data.size() < entrySize) {
      continue;
    }
    auto entry = [&](size_t pos) -> size_t {
      return entrySize == 4 ? reader.readUint32(pos) : reader.readUint16(pos);
    };
    const size_t first = entry(0);
    if (first == 0 || first % entrySize != 0 || !isBitmapAt(data, first)) {
      continue;
    }
    std::vector<size_t> offsets;
    for (size_t pos = 0; pos < first; pos += entrySize) {
      offsets.push_back(entry(pos));
    }
    return offsets;
  }
  return {};
}

std::vector<size_t> readTileChain(const std::vector<uint8_t> &data) {
  constexpr size_t TILE_HEADER_SIZE = 4;
  if (data.size() < TILE_HEADER_SIZE + 2) {
    throw std::runtime_error("Tile file is too small for its header");
  }
  const uint8_t count = data[3];
  if (count == 0) {
    throw std::runtime_error("Tile file has no tiles");
  }

  helpers::BigEndianReader reader(data);
  std::vector<size_t> offsets;
  size_t offset = TILE_HEADER_SIZE + 2;
  for (int i = 0; i < count; i++) {
    if (!isBitmapAt(data, offset)) {
      throw std::runtime_error("Tile chain is broken at tile " +
                               std::to_string(i));
    }
    offsets.push_back(offset);
    offset += reader.readUint16(offset - 2);
  }
  return offsets;
}

bool isTileFile(const std::string &id) {
  return std::find(gameData::fileIds::TILE_FILES.begin(),
                   gameData::fileIds::TILE_FILES.end(),
                   std::string_view(id)) != gameData::fileIds::TILE_FILES.end();
}

std::vector<uint16_t> readSPACKPalette(const std::vector<uint8_t> &data,
                                       size_t offset) {
  if (offset > data.size()) {
    throw std::runtime_error("SPACK palette offset is past the end of data");
  }
  std::vector<uint8_t> slice(data.begin() + static_cast<std::ptrdiff_t>(offset),
                             data.end());
  auto hdr = headers::parseSPACKHeader(slice);
  return {std::begin(hdr.amigaPalette), std::end(hdr.amigaPalette)};
}

std::string skipReason(const DecodedImage &img) {
  if (img.pixels.empty()) {
    return "Bitmap decoded to an empty image";
  }
  return "Bitmap is only " + std::to_string(img.width) + "x" +
         std::to_string(img.height) + " pixels";
}

ExtractedBitmap convertBitmap(const std::vector<uint8_t> &data, size_t offset,
                              const std::vector<uint16_t> &palette,
                              const std::string &name, bool skipTiny) {
  try {
    auto img = decodeAmosBitmap(data, offset, palette.data(),
                                static_cast<int>(palette.size()));
    if (img.pixels.empty() || (skipTiny && (img.width < 2 || img.height < 2))) {
      return {name, {}, skipReason(img)};
    }
    auto bmp = bmpWriter::pixelsToBmp(img.width, img.height, img.pixels.data(),
                                      palette.data(),
                                      static_cast<int>(palette.size()));
    return {name, std::move(bmp), {}};
  } catch (const std::exception &e) {
    return {name, {}, e.what()};
  }
}

std::vector<ExtractedBitmap> extractSCCode(const std::vector<uint8_t> &data,
                                           const std::string &fileId) {
  auto p = readSPACKPalette(data, 0);
  auto offsets = findBMCodeOffsets(data);

  std::vector<ExtractedBitmap> results;
  for (size_t i = 0; i < offsets.size(); i++) {
    std::string name = (i == 0) ? fileId : fileId + "_" + std::to_string(i);
    results.push_back(convertBitmap(data, offsets[i], p, name, false));
  }
  return results;
}

std::vector<ExtractedBitmap> extractTiles(const std::vector<uint8_t> &data,
                                          const std::string &fileId) {
  auto offsets = readTileChain(data);

  const std::vector<uint16_t> palette(pal::LEVEL.begin(), pal::LEVEL.end());
  std::vector<ExtractedBitmap> results;
  for (size_t i = 0; i < offsets.size(); i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%s_%03zu", fileId.c_str(), i);
    results.push_back(convertBitmap(data, offsets[i], palette, buf, false));
  }
  return results;
}

std::vector<ExtractedBitmap>
extractMultiBMCode(const std::vector<uint8_t> &data,
                   const std::string &fileId) {
  auto p = pal::selectPalette(fileId);
  auto offsets = readBitmapTable(data);
  if (offsets.empty()) {
    offsets = findBMCodeOffsets(data);
  }

  std::vector<ExtractedBitmap> results;
  for (size_t i = 0; i < offsets.size(); i++) {
    std::string name = (i == 0) ? fileId : fileId + "_" + std::to_string(i);
    results.push_back(convertBitmap(data, offsets[i], p, name, true));
  }
  return results;
}

std::vector<ExtractedBitmap> extract0384(const std::vector<uint8_t> &data) {
  std::vector<uint16_t> curPal(pal::HUD.begin(), pal::HUD.end());

  std::vector<ExtractedBitmap> results;
  helpers::BigEndianReader reader(data);

  size_t firstImage = data.size();
  for (size_t pos = 0; pos + 2 <= firstImage; pos += 2) {
    size_t offset = reader.readUint16(pos);
    const bool screen = isScreenAt(data, offset);
    if (offset <= pos || (!screen && !isBitmapAt(data, offset))) {
      break;
    }
    firstImage = std::min(firstImage, offset);
    if (screen) {
      curPal = readSPACKPalette(data, offset);
      offset += amosConsts::SPACK_HEADER_SIZE;
    }
    const size_t index = results.size();
    std::string name =
        (index == 0) ? std::string(gameData::fileIds::MULTI_PALETTE_BITMAP)
                     : std::string(gameData::fileIds::MULTI_PALETTE_BITMAP) +
                           "_" + std::to_string(index);
    results.push_back(convertBitmap(data, offset, curPal, name, true));
  }
  return results;
}

} // anonymous namespace

std::vector<ExtractedBitmap> extract(const std::vector<uint8_t> &data,
                                     const std::string &fileId) {
  if (data.size() < 4) {
    throw std::runtime_error("Data too small to extract bitmaps");
  }

  helpers::BigEndianReader reader(data);
  uint32_t magic = reader.readUint32(0);

  if (std::string_view(fileId) == gameData::fileIds::MULTI_PALETTE_BITMAP) {
    return extract0384(data);
  }

  if (magic == amosConsts::SPACK_SCREEN_HEADER &&
      std::string_view(fileId) != gameData::fileIds::HUD_SPRITES) {
    return extractSCCode(data, fileId);
  }

  if (isTileFile(fileId)) {
    return extractTiles(data, fileId);
  }

  return extractMultiBMCode(data, fileId);
}

} // namespace openfranko::lib::converter::bitmapExtractor
