#include "bitmapExtractor.h"
#include "../../bmpWriter/bmpWriter.h"
#include "../../helpers/helpers.h"
#include "../gameData/gameData.h"
#include "../spriteSheet/Palettes.h"
#include "../amosCompact/Consts.h"
#include "../shared/decodeImage.h"
#include "../shared/headers.h"
#include <algorithm>
#include <stdexcept>

namespace openfranko::lib::converter::bitmapExtractor {

namespace amosConsts = amosCompact::consts;
namespace pal = spriteSheet::palettes;
using converter::DecodedImage;
using converter::decodeAmosBitmap;

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

bool isTileFile(const std::string &id) {
  return std::find(gameData::fileIds::TILE_FILES.begin(),
                   gameData::fileIds::TILE_FILES.end(),
                   std::string_view(id)) != gameData::fileIds::TILE_FILES.end();
}

std::vector<uint16_t> readSPACKPalette(const std::vector<uint8_t> &data,
                                       size_t offset) {
  std::vector<uint8_t> slice(data.begin() + offset, data.end());
  auto hdr = headers::parseSPACKHeader(slice);
  return {std::begin(hdr.amigaPalette), std::end(hdr.amigaPalette)};
}

std::vector<ExtractedBitmap> extractSCCode(const std::vector<uint8_t> &data,
                                           const std::string &fileId) {
  auto p = readSPACKPalette(data, 0);
  auto offsets = findBMCodeOffsets(data);

  std::vector<ExtractedBitmap> results;
  for (size_t i = 0; i < offsets.size(); i++) {
    auto img =
        decodeAmosBitmap(data, offsets[i], p.data(), static_cast<int>(p.size()));
    if (img.pixels.empty()) {
      continue;
    }
    std::string name = (i == 0) ? fileId : fileId + "_" + std::to_string(i);
    results.push_back(
        {name, bmpWriter::pixelsToBmp(img.width, img.height, img.pixels.data(),
                                      p.data(), static_cast<int>(p.size()))});
  }
  return results;
}

std::vector<ExtractedBitmap> extractTiles(const std::vector<uint8_t> &data,
                                          const std::string &fileId) {
  auto offsets = findBMCodeOffsets(data);
  if (offsets.empty()) {
    throw std::runtime_error("No bitmap code offsets found");
  }

  std::vector<ExtractedBitmap> results;
  for (size_t i = 0; i < offsets.size(); i++) {
    auto img = decodeAmosBitmap(data, offsets[i], pal::LEVEL.data(),
                                static_cast<int>(pal::LEVEL.size()));
    if (img.pixels.empty()) {
      continue;
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%s_%03zu", fileId.c_str(), i);
    results.push_back(
        {std::string(buf),
         bmpWriter::pixelsToBmp(img.width, img.height, img.pixels.data(),
                                pal::LEVEL.data(),
                                static_cast<int>(pal::LEVEL.size()))});
  }
  return results;
}

std::vector<ExtractedBitmap>
extractMultiBMCode(const std::vector<uint8_t> &data,
                   const std::string &fileId) {
  auto p = pal::selectPalette(fileId);
  auto offsets = findBMCodeOffsets(data);

  std::vector<ExtractedBitmap> results;
  for (size_t i = 0; i < offsets.size(); i++) {
    auto img = decodeAmosBitmap(data, offsets[i], p.data(),
                                static_cast<int>(p.size()));
    if (img.pixels.empty() || img.width < 2 || img.height < 2) {
      continue;
    }
    std::string name = (i == 0) ? fileId : fileId + "_" + std::to_string(i);
    results.push_back(
        {name, bmpWriter::pixelsToBmp(img.width, img.height, img.pixels.data(),
                                      p.data(), static_cast<int>(p.size()))});
  }
  return results;
}

std::vector<ExtractedBitmap> extract0384(const std::vector<uint8_t> &data) {
  std::vector<uint16_t> curPal(pal::HUD.begin(), pal::HUD.end());
  int found = 0;

  std::vector<ExtractedBitmap> results;
  helpers::BigEndianReader reader(data);

  for (size_t off = 0; off + 4 <= data.size(); off += 2) {
    uint32_t magic = reader.readUint32(off);

    if (magic == amosConsts::SPACK_SCREEN_HEADER &&
        off + amosConsts::SPACK_HEADER_SIZE <= data.size()) {
      curPal = readSPACKPalette(data, off);
      continue;
    }

    if (magic == amosConsts::AMOS_BMCODE &&
        off + amosConsts::PACKED_BITMAP_HEADER_SIZE <= data.size()) {
      auto img = decodeAmosBitmap(data, off, curPal.data(),
                                  static_cast<int>(curPal.size()));
      if (img.pixels.empty() || img.width < 2 || img.height < 2) {
        continue;
      }
      std::string name =
          (found == 0) ? std::string(gameData::fileIds::MULTI_PALETTE_BITMAP)
                       : std::string(gameData::fileIds::MULTI_PALETTE_BITMAP) +
                             "_" + std::to_string(found);
      results.push_back(
          {name, bmpWriter::pixelsToBmp(img.width, img.height,
                                        img.pixels.data(), curPal.data(),
                                        static_cast<int>(curPal.size()))});
      found++;
    }
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
