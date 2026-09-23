#include "frankoResourceExtractor.h"
#include "../../lib/converter/abkToS3m/abkToS3m.h"
#include "../../lib/converter/amosCompact/amosCompact.h"
#include "../../lib/converter/audioExtractor/audioExtractor.h"
#include "../../lib/converter/bitmapExtractor/bitmapExtractor.h"
#include "../../lib/converter/fileContainer/fileContainer.h"
#include "../../lib/converter/gameData/gameData.h"
#include "../../lib/converter/levelScript/levelScript.h"
#include "../../lib/converter/spriteSheet/spriteSheet.h"
#include "../../lib/decompressor/backwardLZ77/backwardLZ77.h"
#include "../../lib/filesystem/readFile/readFile.h"
#include "../../lib/filesystem/writeFile/writeFile.h"
#include "../../lib/helpers/helpers.h"
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string_view>
#include <vector>

namespace openfranko::tools::converter::frankoResourceExtractor {

namespace lib = openfranko::lib;

namespace {

size_t hashSamBank(const std::vector<uint8_t> &data) {
  if (data.size() < 12) {
    return 0;
  }
  uint32_t sbOff = lib::helpers::BigEndianReader(data).readUint32(8);
  if (sbOff == 0 || sbOff >= data.size()) {
    return 0;
  }
  size_t h = 0;
  for (size_t i = sbOff; i < data.size(); i++) {
    h ^= std::hash<uint8_t>{}(data[i]) + 0x9e3779b9 + (h << 6) + (h >> 2);
  }
  return h;
}

std::string fileIdToHex(uint16_t id) {
  std::ostringstream ss;
  ss << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << id;
  return ss.str();
}

bool isLevelFile(const std::string &fileId) {
  const auto &levelFiles = lib::converter::gameData::fileIds::LEVEL_FILES;
  return std::find(levelFiles.begin(), levelFiles.end(),
                   std::string_view(fileId)) != levelFiles.end();
}

struct OutputFile {
  std::string name;
  std::vector<uint8_t> data;
};

void writeOutputs(const std::string &outDir, const std::string &fileId,
                  const std::vector<OutputFile> &outputs) {
  if (outputs.empty()) {
    return;
  }
  std::string dir = outDir;
  if (outputs.size() > 1) {
    dir = outDir + "/" + fileId;
    std::filesystem::create_directories(dir);
  }
  for (const auto &f : outputs) {
    std::string path = dir + "/" + f.name;
    lib::filesystem::writeFile::writeFile(path, f.data);
    std::cerr << "  -> " << path << " (" << f.data.size() << " bytes)"
              << std::endl;
  }
}

void patch0038SunsetBitmap(std::vector<uint8_t> &bmp) {
  if (bmp.size() < 54 + 4 * 3) {
    return;
  }

  auto setPaletteEntry = [&](int index, uint8_t r, uint8_t g, uint8_t b) {
    size_t offset = 54 + static_cast<size_t>(index) * 4;
    if (offset + 4 > bmp.size()) {
      return;
    }
    bmp[offset + 0] = b;
    bmp[offset + 1] = g;
    bmp[offset + 2] = r;
    bmp[offset + 3] = 0;
  };

  setPaletteEntry(1, 0xFF, 0xFF, 0xFF);
  setPaletteEntry(2, 0x77, 0x77, 0x77);
}

} // namespace

int validateDirectory(const std::string &dirPath) {
  int missing = 0;
  for (const auto name : lib::converter::gameData::fileIds::EXPECTED_FILES) {
    std::string path = dirPath + "/" + std::string(name);
    if (!std::filesystem::exists(path)) {
      std::cerr << "Missing file: " << path << std::endl;
      missing++;
    }
  }
  return missing;
}

int processFile(const std::string &inputPath, const std::string &outDir,
                std::set<size_t> &seenSamBanks) {
  auto rawData = lib::filesystem::readFile::readFile(inputPath);
  auto info = lib::converter::fileContainer::parseFooter(rawData);
  std::string fileId = fileIdToHex(info.fileId);

  std::cerr << fileId << " ["
            << lib::converter::gameData::resourceTypes::name(info.resourceType)
            << "] " << rawData.size() << " bytes" << std::endl;

  std::vector<OutputFile> outputs;

  if (info.resourceType ==
      lib::converter::gameData::resourceTypes::SCREEN_PACKAGE) {
    try {
      auto bmpData = lib::converter::amosCompact::decompress(rawData);
      outputs.push_back({fileId + ".bmp", std::move(bmpData)});
    } catch (const std::exception &e) {
      std::cerr << "  SPACK error: " << e.what() << std::endl;
    }
    writeOutputs(outDir, fileId, outputs);
    return 0;
  }

  std::vector<uint8_t> dec;
  try {
    dec = lib::decompressor::backwardLZ77::decompress(rawData);
  } catch (const std::exception &e) {
    std::cerr << "  LZ77 error: " << e.what() << std::endl;
    return 1;
  }

  std::cerr << "  decompressed: " << dec.size() << " bytes" << std::endl;

  switch (info.resourceType) {
  case lib::converter::gameData::resourceTypes::SPRITES: {
    try {
      auto palette = lib::converter::spriteSheet::selectPalette(fileId);
      auto sprites =
          lib::converter::spriteSheet::convertToIndividual(dec, palette);
      int idx = 0;
      for (auto &sprite : sprites) {
        if (!sprite.empty()) {
          if (fileId == "0038" && idx >= 43 && idx <= 100) {
            patch0038SunsetBitmap(sprite);
          }
          char buf[32];
          snprintf(buf, sizeof(buf), "%s_%03d.bmp", fileId.c_str(), idx);
          std::string bmpName(buf);
          outputs.push_back({bmpName, std::move(sprite)});
        }
        idx++;
      }
    } catch (const std::exception &e) {
      std::cerr << "  sprite error: " << e.what() << std::endl;
    }
    {
      size_t samHash = hashSamBank(dec);
      if (samHash != 0 && seenSamBanks.insert(samHash).second) {
        auto samples =
            lib::converter::audioExtractor::extractEmbeddedSamBank(dec, fileId);
        for (auto &s : samples) {
          outputs.push_back({std::move(s.name), std::move(s.data)});
        }
      }
    }
    break;
  }

  case lib::converter::gameData::resourceTypes::ICONS: {
    if (isLevelFile(fileId)) {
      try {
        auto level = lib::converter::levelScript::parse(dec);
        outputs.push_back({fileId + ".json",
                           lib::converter::levelScript::toJson(level, fileId)});
      } catch (const std::exception &e) {
        std::cerr << "  level script error: " << e.what() << std::endl;
      }
      break;
    }
    auto bitmaps = lib::converter::bitmapExtractor::extract(dec, fileId);
    for (auto &bm : bitmaps) {
      outputs.push_back({bm.name + ".bmp", std::move(bm.bmpData)});
    }
    if (bitmaps.empty()) {
      std::cerr << "  (no bitmaps extracted)" << std::endl;
    }
    break;
  }

  case lib::converter::gameData::resourceTypes::SAMPLES: {
    auto samples =
        lib::converter::audioExtractor::extractStandaloneSamBank(dec, fileId);
    for (auto &s : samples) {
      outputs.push_back({std::move(s.name), std::move(s.data)});
    }
    if (samples.empty()) {
      std::cerr << "  (no samples extracted)" << std::endl;
    }
    break;
  }

  case lib::converter::gameData::resourceTypes::MUSIC: {
    auto abk = lib::converter::audioExtractor::wrapMusicBank(dec, fileId);
    auto s3mData = lib::converter::abkToS3m::convert(abk.data);
    outputs.push_back({fileId + ".s3m", std::move(s3mData)});
    break;
  }

  default:
    std::cerr << "  unknown resource type 0x" << std::hex << info.resourceType
              << std::dec << std::endl;
    break;
  }

  writeOutputs(outDir, fileId, outputs);
  return 0;
}

} // namespace openfranko::tools::converter::frankoResourceExtractor
