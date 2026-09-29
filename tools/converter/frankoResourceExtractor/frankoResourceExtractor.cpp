#include "frankoResourceExtractor.h"

#include "../../../lib/converter/abkToS3m/abkToS3m.h"
#include "../../../lib/converter/amosCompact/amosCompact.h"
#include "../../../lib/converter/audioExtractor/audioExtractor.h"
#include "../../../lib/converter/bitmapExtractor/bitmapExtractor.h"
#include "../../../lib/converter/codeCards/codeCards.h"
#include "../../../lib/converter/endingCredits/endingCredits.h"
#include "../../../lib/converter/fileContainer/fileContainer.h"
#include "../../../lib/converter/gameData/gameData.h"
#include "../../../lib/converter/gameData/palettes.h"
#include "../../../lib/converter/levelScript/levelScript.h"
#include "../../../lib/converter/spriteSheet/spriteSheet.h"
#include "../../../lib/filesystem/readFile/readFile.h"
#include "../../../lib/filesystem/writeFile/writeFile.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <vector>

namespace openfranko::tools::converter::frankoResourceExtractor {
namespace {

bool isLevelFile(const std::string &fileId) {
  const auto &levelFiles = lib::converter::gameData::fileIds::LEVEL_FILES;
  return std::find(levelFiles.begin(), levelFiles.end(),
                   lib::converter::gameData::version10Id(fileId)) !=
         levelFiles.end();
}

bool isVersion12(const std::string &fileId) {
  return lib::converter::gameData::version12::find(fileId) != nullptr;
}

bool holdsVersion12(const std::string &inputDir) {
  const auto present = [&inputDir](std::string_view name) {
    return std::filesystem::exists(std::filesystem::path(inputDir) /
                                   std::string(name));
  };
  const auto version10Files = std::count_if(
      lib::converter::gameData::fileIds::EXPECTED_FILES.begin(),
      lib::converter::gameData::fileIds::EXPECTED_FILES.end(), present);
  const auto version12Files = std::count_if(
      lib::converter::gameData::version12::FILES.begin(),
      lib::converter::gameData::version12::FILES.end(),
      [&present](const auto &file) { return present(file.name); });
  return version12Files > version10Files;
}

std::vector<std::string_view> expectedFiles(const std::string &inputDir) {
  if (!holdsVersion12(inputDir)) {
    return {lib::converter::gameData::fileIds::EXPECTED_FILES.begin(),
            lib::converter::gameData::fileIds::EXPECTED_FILES.end()};
  }
  std::vector<std::string_view> names;
  for (const auto &file : lib::converter::gameData::version12::FILES) {
    names.push_back(file.name);
  }
  return names;
}

bool isHexName(const std::string &name) {
  return name.size() == 4 && std::all_of(name.begin(), name.end(), [](char c) {
           return std::isxdigit(static_cast<unsigned char>(c)) != 0;
         });
}

void applyScreenPalette(
    const std::string &inputPath, const std::string &fileId,
    std::vector<lib::converter::spriteSheet::ConvertedSprite> &sprites) {
  const std::string screen(lib::converter::spriteSheet::paletteScreen(fileId));
  if (screen.empty()) {
    return;
  }
  const auto screenPath =
      std::filesystem::path(inputPath).parent_path() / screen;
  try {
    const auto bank = lib::converter::fileContainer::unpack(
        screen, lib::filesystem::readFile::readFile(screenPath.string()));
    lib::converter::spriteSheet::applyScreenPalette(fileId, bank.data, sprites);
  } catch (const std::exception &e) {
    std::cerr << "  Warning: " << e.what() << ": the sprites shown on "
              << screen << " keep the bank's palette" << std::endl;
  }
}

struct OutputFile {
  std::string name;
  std::vector<uint8_t> data;
};

void writeOutputs(const std::string &outputDir, const std::string &fileId,
                  const std::vector<OutputFile> &outputs) {
  if (outputs.empty()) {
    return;
  }
  std::string directory = outputDir;
  if (outputs.size() > 1) {
    directory = outputDir + "/" + fileId;
    std::filesystem::create_directories(directory);
  }
  for (const auto &output : outputs) {
    const std::string path = directory + "/" + output.name;
    lib::filesystem::writeFile::writeFile(path, output.data);
    std::cerr << "  Wrote " << path << " (" << output.data.size() << " bytes)"
              << std::endl;
  }
}

} // namespace

std::vector<std::string> dataFiles(const std::string &inputDir) {
  std::vector<std::string> files;
  if (holdsVersion12(inputDir)) {
    for (const auto &file : lib::converter::gameData::version12::FILES) {
      const auto path =
          std::filesystem::path(inputDir) / std::string(file.name);
      if (std::filesystem::is_regular_file(path)) {
        files.push_back(path.string());
      }
    }
    return files;
  }
  for (const auto &entry : std::filesystem::directory_iterator(inputDir)) {
    if (entry.is_regular_file() &&
        isHexName(entry.path().filename().string())) {
      files.push_back(entry.path().string());
    }
  }
  std::sort(files.begin(), files.end());
  return files;
}

int validateDirectory(const std::string &inputDir) {
  int missing = 0;
  for (const auto name : expectedFiles(inputDir)) {
    const std::string path = inputDir + "/" + std::string(name);
    if (!std::filesystem::exists(path)) {
      std::cerr << "Missing file: " << path << std::endl;
      ++missing;
    }
  }
  return missing;
}

namespace {

int extractFile(const std::string &inputPath, const std::string &outputDir) {
  const auto raw = lib::filesystem::readFile::readFile(inputPath);
  const auto resource = lib::converter::fileContainer::unpack(
      std::filesystem::path(inputPath).filename().string(), raw);
  const std::string &fileId = resource.fileId;

  std::cerr << fileId << " ["
            << lib::converter::gameData::resourceTypes::name(
                   resource.resourceType)
            << "]" << std::endl;
  std::cerr << "  Read " << raw.size() << " bytes" << std::endl;

  std::vector<OutputFile> outputs;
  bool failed = false;

  if (resource.resourceType ==
      lib::converter::gameData::resourceTypes::SCREEN_PACKAGE) {
    try {
      auto bmp = lib::converter::amosCompact::decompress(resource.data);
      std::cerr << "  Decompressed to " << bmp.size() << " bytes" << std::endl;
      outputs.push_back({fileId + ".bmp", std::move(bmp)});
    } catch (const std::exception &e) {
      std::cerr << "  SPACK error: " << e.what() << std::endl;
      failed = true;
    }
    writeOutputs(outputDir, fileId, outputs);
    return failed ? 1 : 0;
  }

  const auto &decompressed = resource.data;
  std::cerr << "  Decompressed to " << decompressed.size() << " bytes"
            << std::endl;

  switch (resource.resourceType) {
  case lib::converter::gameData::resourceTypes::SPRITES: {
    try {
      const auto palette =
          lib::converter::gameData::palettes::selectPalette(fileId);
      auto sprites = lib::converter::spriteSheet::convertToIndividual(
          decompressed, palette);
      lib::converter::spriteSheet::applySpritePaletteFixes(fileId, sprites);
      applyScreenPalette(inputPath, fileId, sprites);
      for (int i = 0; i < static_cast<int>(sprites.size()); ++i) {
        if (sprites[i].data.empty()) {
          std::cerr << "  Skipped sprite " << i << ": " << sprites[i].error
                    << std::endl;
          continue;
        }
        char name[32];
        snprintf(name, sizeof(name), "%s_%03d.bmp", fileId.c_str(), i);
        outputs.push_back({name, std::move(sprites[i].data)});
      }
    } catch (const std::exception &e) {
      std::cerr << "  Sprite error: " << e.what() << std::endl;
      failed = true;
    }
    try {
      auto samples = lib::converter::audioExtractor::extractEmbeddedSamBank(
          decompressed, fileId);
      for (auto &sample : samples) {
        outputs.push_back({std::move(sample.name), std::move(sample.data)});
      }
    } catch (const std::exception &e) {
      std::cerr << "  Sample error: " << e.what() << std::endl;
      failed = true;
    }
    break;
  }

  case lib::converter::gameData::resourceTypes::ICONS: {
    if (isLevelFile(fileId)) {
      try {
        const auto level = lib::converter::levelScript::parse(decompressed);
        outputs.push_back({fileId + ".json",
                           lib::converter::levelScript::toJson(level, fileId)});
      } catch (const std::exception &e) {
        std::cerr << "  Level script error: " << e.what() << std::endl;
        failed = true;
      }
      break;
    }
    try {
      auto bitmaps =
          lib::converter::bitmapExtractor::extract(decompressed, fileId);
      for (auto &bitmap : bitmaps) {
        if (!bitmap.error.empty()) {
          std::cerr << "  Skipped " << bitmap.name << ": " << bitmap.error
                    << std::endl;
          continue;
        }
        outputs.push_back({bitmap.name + ".bmp", std::move(bitmap.data)});
      }
      if (outputs.empty()) {
        std::cerr << "  No bitmaps extracted." << std::endl;
      }
    } catch (const std::exception &e) {
      std::cerr << "  Bitmap error: " << e.what() << std::endl;
      failed = true;
    }
    if (lib::converter::gameData::version10Id(fileId) ==
        lib::converter::gameData::fileIds::CODE_CARDS) {
      const std::size_t cardSize =
          isVersion12(fileId)
              ? lib::converter::codeCards::consts::VERSION12_CARD_SIZE
              : lib::converter::codeCards::consts::CARD_SIZE;
      try {
        const auto cards =
            lib::converter::codeCards::parse(decompressed, cardSize);
        outputs.push_back({fileId + "_codecards.json",
                           lib::converter::codeCards::toJson(cards)});
      } catch (const std::exception &e) {
        std::cerr << "  Code card error: " << e.what() << std::endl;
        failed = true;
      }
      const std::size_t start =
          lib::converter::codeCards::consts::FIRST_CARD_OFFSET;
      const std::size_t end =
          start +
          lib::converter::codeCards::consts::CARD_COUNT * cardSize * cardSize;
      if (decompressed.size() >= end) {
        outputs.push_back({fileId + "_cards.bin",
                           std::vector<uint8_t>(decompressed.begin() + start,
                                                decompressed.begin() + end)});
      }
    }
    break;
  }

  case lib::converter::gameData::resourceTypes::SAMPLES: {
    try {
      auto samples = lib::converter::audioExtractor::extractStandaloneSamBank(
          decompressed, fileId);
      for (auto &sample : samples) {
        outputs.push_back({std::move(sample.name), std::move(sample.data)});
      }
      if (samples.empty()) {
        std::cerr << "  No samples extracted." << std::endl;
      }
    } catch (const std::exception &e) {
      std::cerr << "  Sample error: " << e.what() << std::endl;
      failed = true;
    }
    break;
  }

  case lib::converter::gameData::resourceTypes::MUSIC: {
    try {
      const auto abk =
          lib::converter::audioExtractor::wrapMusicBank(decompressed, fileId);
      auto s3m = lib::converter::abkToS3m::convert(abk.data);
      outputs.push_back({fileId + ".s3m", std::move(s3m)});
    } catch (const std::exception &e) {
      std::cerr << "  Music error: " << e.what() << std::endl;
      failed = true;
    }
    break;
  }

  default:
    std::cerr << "  Unknown resource type 0x" << std::hex
              << resource.resourceType << std::dec << std::endl;
    break;
  }

  writeOutputs(outputDir, fileId, outputs);
  return failed ? 1 : 0;
}

} // namespace

int processFile(const std::string &inputPath, const std::string &outputDir) {
  try {
    return extractFile(inputPath, outputDir);
  } catch (const std::exception &e) {
    std::cerr << "Error: " << inputPath << ": " << e.what() << std::endl;
    return 1;
  }
}

int processExecutable(const std::string &inputPath,
                      const std::string &outputDir) {
  try {
    const auto executable = lib::filesystem::readFile::readFile(inputPath);
    std::cerr << std::filesystem::path(inputPath).filename().string()
              << " [Executable]" << std::endl;
    std::cerr << "  Read " << executable.size() << " bytes" << std::endl;

    const auto pages = lib::converter::endingCredits::extract(executable);
    std::cerr << "  Ending credits: " << pages.size() << " pages" << std::endl;
    const auto json = lib::converter::endingCredits::toJson(pages);
    const std::string outputPath =
        (std::filesystem::path(outputDir) / "credits.json").string();
    lib::filesystem::writeFile::writeFile(outputPath, json);
    std::cerr << "  Wrote " << outputPath << " (" << json.size() << " bytes)"
              << std::endl;

    const auto intro = lib::converter::endingCredits::extractIntro(executable);
    if (!intro.empty()) {
      std::cerr << "  Intro texts: " << intro.size() << " pages" << std::endl;
      const auto introJson = lib::converter::endingCredits::toJson(intro);
      const std::string introPath =
          (std::filesystem::path(outputDir) / "intro.json").string();
      lib::filesystem::writeFile::writeFile(introPath, introJson);
      std::cerr << "  Wrote " << introPath << " (" << introJson.size()
                << " bytes)" << std::endl;
    }
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << inputPath << ": " << e.what() << std::endl;
    return 1;
  }
}

} // namespace openfranko::tools::converter::frankoResourceExtractor
