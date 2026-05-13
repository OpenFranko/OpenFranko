#include "../../lib/argumentParser/ArgumentParser.h"
#include "../../lib/converter/abkToS3m/abkToS3m.h"
#include "../../lib/converter/audioExtractor/audioExtractor.h"
#include "../../lib/converter/bitmapExtractor/bitmapExtractor.h"
#include "../../lib/converter/fileContainer/fileContainer.h"
#include "../../lib/converter/spriteSheet/spriteSheet.h"
#include "../../lib/converter/amosCompact/amosCompact.h"
#include "../../lib/decompressor/backwardLZ77/backwardLZ77.h"
#include "../../lib/decompressor/helpers/helpers.h"
#include "../../lib/filesystem/readFile/readFile.h"
#include "../../lib/filesystem/writeFile/writeFile.h"
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>

using namespace openfranko::lib;

static size_t hashSamBank(const std::vector<uint8_t> &data) {
  if (data.size() < 12) {
    return 0;
  }
  uint32_t sbOff = decompressor::helpers::readUint32BigEndian(data, 8);
  if (sbOff == 0 || sbOff >= data.size()) {
    return 0;
  }
  size_t h = 0;
  for (size_t i = sbOff; i < data.size(); i++) {
    h ^= std::hash<uint8_t>{}(data[i]) + 0x9e3779b9 + (h << 6) + (h >> 2);
  }
  return h;
}

static std::string fileIdToHex(uint16_t id) {
  std::ostringstream ss;
  ss << std::uppercase << std::setfill('0') << std::setw(4) << std::hex << id;
  return ss.str();
}

static std::string resourceTypeName(uint16_t type) {
  switch (type) {
  case 0x0000:
    return "Sprites";
  case 0x0200:
    return "Icons";
  case 0x0201:
    return "ScreenPkg";
  case 0x0300:
    return "Samples";
  case 0x0400:
    return "Music";
  default:
    return "Unknown";
  }
}

static void writeOutput(const std::string &dir, const std::string &name,
                        const std::vector<uint8_t> &data) {
  std::string path = dir + "/" + name;
  filesystem::writeFile::writeFile(path, data);
  std::cerr << "  -> " << path << " (" << data.size() << " bytes)" << std::endl;
}

static int processFile(const std::string &inputPath,
                       const std::string &outDir,
                       std::set<size_t> &seenSamBanks) {
  auto rawData = filesystem::readFile::readFile(inputPath);
  auto info = converter::fileContainer::parseFooter(rawData);
  std::string fileId = fileIdToHex(info.fileId);

  std::cerr << fileId << " [" << resourceTypeName(info.resourceType) << "] "
            << rawData.size() << " bytes" << std::endl;

  if (info.resourceType == 0x0201) {
    try {
      auto bmpData = converter::amosCompact::decompress(rawData);
      writeOutput(outDir, fileId + ".bmp", bmpData);
    } catch (const std::exception &e) {
      std::cerr << "  SPACK error: " << e.what() << std::endl;
    }
    return 0;
  }

  std::vector<uint8_t> dec;
  try {
    dec = decompressor::backwardLZ77::decompress(rawData);
  } catch (const std::exception &e) {
    std::cerr << "  LZ77 error: " << e.what() << std::endl;
    return 1;
  }

  std::cerr << "  decompressed: " << dec.size() << " bytes" << std::endl;

  switch (info.resourceType) {
  case 0x0000: {
    try {
      auto palette = converter::spriteSheet::selectPalette(fileId);
      auto bmps = converter::spriteSheet::convertToIndividual(dec, palette);
      int idx = 0;
      for (const auto &bmp : bmps) {
        if (!bmp.empty()) {
          char buf[32];
          snprintf(buf, sizeof(buf), "%s_%03d.bmp", fileId.c_str(), idx);
          writeOutput(outDir, std::string(buf), bmp);
        }
        idx++;
      }
    } catch (const std::exception &e) {
      std::cerr << "  sprite error: " << e.what() << std::endl;
    }
    {
      size_t samHash = hashSamBank(dec);
      if (samHash != 0 && seenSamBanks.insert(samHash).second) {
        auto samples = converter::audioExtractor::extractEmbeddedSamBank(dec, fileId);
        for (const auto &s : samples) {
          writeOutput(outDir, s.name, s.data);
        }
      }
    }
    break;
  }

  case 0x0200: {
    auto bitmaps = converter::bitmapExtractor::extract(dec, fileId);
    for (const auto &bm : bitmaps) {
      writeOutput(outDir, bm.name + ".bmp", bm.bmpData);
    }
    if (bitmaps.empty()) {
      std::cerr << "  (no bitmaps extracted)" << std::endl;
    }
    break;
  }

  case 0x0300: {
    auto samples = converter::audioExtractor::extractStandaloneSamBank(dec, fileId);
    for (const auto &s : samples) {
      writeOutput(outDir, s.name, s.data);
    }
    if (samples.empty()) {
      std::cerr << "  (no samples extracted)" << std::endl;
    }
    break;
  }

  case 0x0400: {
    auto abk = converter::audioExtractor::wrapMusicBank(dec, fileId);
    auto s3mData = converter::abkToS3m::convert(abk.data);
    std::string s3mName = fileId + ".s3m";
    writeOutput(outDir, s3mName, s3mData);
    break;
  }

  default:
    std::cerr << "  unknown resource type 0x" << std::hex << info.resourceType
              << std::dec << std::endl;
    break;
  }

  return 0;
}

int main(int argc, char **argv) {
  argumentParser::ArgumentParser parser(argc, argv);

  const auto inputOptional = parser.getCmdOption("-i");
  if (!inputOptional.has_value()) {
    std::cerr << "Usage: " << argv[0] << " -i <file_or_dir> [-o <output_dir>]"
              << std::endl;
    std::cerr << "Extracts all Franko game data files to standard formats."
              << std::endl;
    std::cerr << "  Sprites (0x0000) -> sheet BMP + embedded WAV samples"
              << std::endl;
    std::cerr << "  Icons   (0x0200) -> BMP (screens, tiles, bitmaps)"
              << std::endl;
    std::cerr << "  Samples (0x0300) -> WAV" << std::endl;
    std::cerr << "  Music   (0x0400) -> S3M (ScreamTracker 3)" << std::endl;
    std::cerr << "  Screen  (0x0201) -> BMP (raw SPACK)" << std::endl;
    return 1;
  }

  std::string inputPath = inputOptional.value();
  std::string outDir = "extracted";
  const auto outputOptional = parser.getCmdOption("-o");
  if (outputOptional.has_value()) {
    outDir = outputOptional.value();
  }

  std::filesystem::create_directories(outDir);

  int errors = 0;

  if (std::filesystem::is_directory(inputPath)) {
    std::vector<std::string> files;
    for (const auto &entry : std::filesystem::directory_iterator(inputPath)) {
      if (entry.is_regular_file()) {
        std::string name = entry.path().filename().string();
        if (name.size() == 4) {
          bool isHex = true;
          for (char c : name) {
            if (!std::isxdigit(static_cast<unsigned char>(c))) {
              isHex = false;
              break;
            }
          }
          if (isHex) {
            files.push_back(entry.path().string());
          }
        }
      }
    }
    std::sort(files.begin(), files.end());

    std::set<size_t> seenSamBanks;
    std::cerr << "Processing " << files.size() << " data files..." << std::endl;
    for (const auto &f : files) {
      errors += processFile(f, outDir, seenSamBanks);
    }

    std::cerr << "\nDone. " << files.size() << " files processed, " << errors
              << " errors." << std::endl;
  } else {
    std::set<size_t> seenSamBanks;
    errors = processFile(inputPath, outDir, seenSamBanks);
  }

  return errors > 0 ? 1 : 0;
}
