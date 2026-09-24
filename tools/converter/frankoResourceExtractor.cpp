#include "frankoResourceExtractor.h"
#include "../../lib/converter/abkToS3m/abkToS3m.h"
#include "../../lib/converter/amosCompact/amosCompact.h"
#include "../../lib/converter/audioExtractor/audioExtractor.h"
#include "../../lib/converter/bitmapExtractor/bitmapExtractor.h"
#include "../../lib/converter/codeCards/codeCards.h"
#include "../../lib/converter/endingCredits/endingCredits.h"
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
#include <iostream>
#include <string_view>
#include <vector>

namespace openfranko::tools::converter::frankoResourceExtractor {

namespace lib = openfranko::lib;

namespace {

std::vector<uint8_t> embeddedSamBank(const std::vector<uint8_t> &data) {
  if (data.size() < 12) {
    return {};
  }
  uint32_t sbOff = lib::helpers::BigEndianReader(data).readUint32(8);
  if (sbOff == 0 || sbOff >= data.size()) {
    return {};
  }
  return {data.begin() + static_cast<std::ptrdiff_t>(sbOff), data.end()};
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

namespace {

int extractFile(const std::string &inputPath, const std::string &outDir) {
  auto rawData = lib::filesystem::readFile::readFile(inputPath);
  auto info = lib::converter::fileContainer::parseFooter(rawData);
  std::string fileId = lib::converter::fileContainer::fileIdToHex(info.fileId);

  std::cerr << fileId << " ["
            << lib::converter::gameData::resourceTypes::name(info.resourceType)
            << "] " << rawData.size() << " bytes" << std::endl;

  std::vector<OutputFile> outputs;
  bool failed = false;

  if (info.resourceType ==
      lib::converter::gameData::resourceTypes::SCREEN_PACKAGE) {
    try {
      auto bmpData = lib::converter::amosCompact::decompress(rawData);
      outputs.push_back({fileId + ".bmp", std::move(bmpData)});
    } catch (const std::exception &e) {
      std::cerr << "  SPACK error: " << e.what() << std::endl;
      failed = true;
    }
    writeOutputs(outDir, fileId, outputs);
    return failed ? 1 : 0;
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
      lib::converter::spriteSheet::applySpritePaletteFixes(fileId, sprites);
      std::vector<int> skipped;
      int idx = 0;
      for (auto &sprite : sprites) {
        if (!sprite.bmpData.empty()) {
          char buf[32];
          snprintf(buf, sizeof(buf), "%s_%03d.bmp", fileId.c_str(), idx);
          std::string bmpName(buf);
          outputs.push_back({bmpName, std::move(sprite.bmpData)});
        } else {
          skipped.push_back(idx);
        }
        idx++;
      }
      if (!skipped.empty()) {
        std::cerr << "  skipped " << skipped.size() << " of " << sprites.size()
                  << " sprites that could not be decoded:" << std::endl;
        for (int i : skipped) {
          std::cerr << "    " << i << ": " << sprites[i].error << std::endl;
        }
      }
    } catch (const std::exception &e) {
      std::cerr << "  sprite error: " << e.what() << std::endl;
      failed = true;
    }
    try {
      auto samBank = embeddedSamBank(dec);
      if (!samBank.empty()) {
        auto samples =
            lib::converter::audioExtractor::extractEmbeddedSamBank(dec, fileId);
        for (auto &s : samples) {
          outputs.push_back({std::move(s.name), std::move(s.data)});
        }
      }
    } catch (const std::exception &e) {
      std::cerr << "  sample error: " << e.what() << std::endl;
      failed = true;
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
        failed = true;
      }
      break;
    }
    try {
      auto bitmaps = lib::converter::bitmapExtractor::extract(dec, fileId);
      size_t skipped = 0;
      for (auto &bm : bitmaps) {
        if (bm.error.empty()) {
          outputs.push_back({bm.name + ".bmp", std::move(bm.bmpData)});
        } else {
          skipped++;
        }
      }
      if (skipped > 0) {
        std::cerr << "  skipped " << skipped << " of " << bitmaps.size()
                  << " bitmaps:" << std::endl;
        for (const auto &bm : bitmaps) {
          if (!bm.error.empty()) {
            std::cerr << "    " << bm.name << ": " << bm.error << std::endl;
          }
        }
      }
      if (skipped == bitmaps.size()) {
        std::cerr << "  (no bitmaps extracted)" << std::endl;
      }
    } catch (const std::exception &e) {
      std::cerr << "  bitmap error: " << e.what() << std::endl;
      failed = true;
    }
    if (std::string_view(fileId) ==
        lib::converter::gameData::fileIds::CODE_CARDS) {
      try {
        auto cards = lib::converter::codeCards::parse(dec);
        outputs.push_back({fileId + "_codecards.json",
                           lib::converter::codeCards::toJson(cards)});
      } catch (const std::exception &e) {
        std::cerr << "  code cards error: " << e.what() << std::endl;
        failed = true;
      }
    }
    namespace cards = lib::converter::gameData::protectionCards;
    if (fileId == cards::FILE_ID && dec.size() >= cards::OFFSET + cards::SIZE) {
      outputs.push_back(
          {fileId + "_cards.bin",
           std::vector<uint8_t>(dec.begin() + cards::OFFSET,
                                dec.begin() + cards::OFFSET + cards::SIZE)});
    }
    break;
  }

  case lib::converter::gameData::resourceTypes::SAMPLES: {
    try {
      auto samples =
          lib::converter::audioExtractor::extractStandaloneSamBank(dec, fileId);
      for (auto &s : samples) {
        outputs.push_back({std::move(s.name), std::move(s.data)});
      }
      if (samples.empty()) {
        std::cerr << "  (no samples extracted)" << std::endl;
      }
    } catch (const std::exception &e) {
      std::cerr << "  sample error: " << e.what() << std::endl;
      failed = true;
    }
    break;
  }

  case lib::converter::gameData::resourceTypes::MUSIC: {
    try {
      auto abk = lib::converter::audioExtractor::wrapMusicBank(dec, fileId);
      auto s3mData = lib::converter::abkToS3m::convert(abk.data);
      outputs.push_back({fileId + ".s3m", std::move(s3mData)});
    } catch (const std::exception &e) {
      std::cerr << "  music error: " << e.what() << std::endl;
      failed = true;
    }
    break;
  }

  default:
    std::cerr << "  unknown resource type 0x" << std::hex << info.resourceType
              << std::dec << std::endl;
    break;
  }

  writeOutputs(outDir, fileId, outputs);
  return failed ? 1 : 0;
}

} // namespace

int processFile(const std::string &inputPath, const std::string &outDir) {
  try {
    return extractFile(inputPath, outDir);
  } catch (const std::exception &e) {
    std::cerr << inputPath << ": " << e.what() << std::endl;
    return 1;
  }
}

int processExecutable(const std::string &inputPath, const std::string &outDir) {
  try {
    const auto pages = lib::converter::endingCredits::extract(
        lib::filesystem::readFile::readFile(inputPath));
    const std::string outputPath =
        (std::filesystem::path(outDir) / "credits.json").string();
    lib::filesystem::writeFile::writeFile(
        outputPath, lib::converter::endingCredits::toJson(pages));
    std::cerr << inputPath << ": ending credits, " << pages.size()
              << " pages -> " << outputPath << std::endl;
    return 0;
  } catch (const std::exception &e) {
    std::cerr << inputPath << ": " << e.what() << std::endl;
    return 1;
  }
}

} // namespace openfranko::tools::converter::frankoResourceExtractor
